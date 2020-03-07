#include "hybrid.h"

#include <memory>
#include <unordered_map>
#include <vector>

#include "container_utils.h"
#include "math/halton.h"
#include "renderer.h"
#include "scene.h"

namespace rei {

namespace hybrid {

struct ViewportData {
  SystemWindowID wnd_id;

  size_t width = 0;
  size_t height = 0;
  SwapchainHandle swapchain;
  BufferHandle depth_stencil_buffer;
  BufferHandle gbuffer0;
  BufferHandle gbuffer1;
  BufferHandle gbuffer2;
  bool gbuffer_dirty = true;

  ShaderArgumentHandle base_shading_inout_arg;
  ShaderArgumentHandle direct_lighting_inout_arg;

  // Multi-bounce GI
  BufferHandle raytracing_output_buffer;
  ShaderArgumentHandle blit_raytracing_output;

  struct AreaLightHandles {
    BufferHandle unshadowed;
    ShaderArgumentHandle unshadowed_pass_arg;
    ShaderArgumentHandle blit_unshadowed;
  } area_light;

  // TAA resources
  BufferHandle taa_cb;
  BufferHandle taa_buffer[2];
  ShaderArgumentHandle taa_argument[2];

  BufferHandle deferred_shading_output;
  ShaderArgumentHandle blit_for_present;

  Vec4 cam_pos = {0, 1, 8, 1};
  Mat4 proj = Mat4::I();
  Mat4 proj_inv = Mat4::I();
  Mat4 view_proj = Mat4::I();
  Mat4 view_proj_inv = Mat4::I();
  unsigned short frame_id = 0;
  bool view_proj_dirty = true;

  void advance_frame() {
    frame_id++;
    if (frame_id == 0) frame_id += 2;
    // reset dirty mark
    view_proj_dirty = false;
    gbuffer_dirty = false;
  }

  ShaderArgumentHandle taa_curr_arg() { return taa_argument[frame_id % 2]; }
  BufferHandle taa_curr_input() { return taa_buffer[frame_id % 2]; }
  BufferHandle taa_curr_output() { return taa_buffer[(frame_id + 1) % 2]; }

  const Mat4& get_view_proj(bool jiterred) {
    if (jiterred) {
      float rndx = HaltonSequence::sample<2>(frame_id);
      float rndy = HaltonSequence::sample<3>(frame_id);
      const Mat4 subpixel_jitter {
        1, 0, 0, (rndx * 2 - 1) / real_t(width),  //
        0, 1, 0, (rndy * 2 - 1) / real_t(height), //
        0, 0, 1, 0,                               //
        0, 0, 0, 1                                //
      };
      return subpixel_jitter * view_proj;
    }
    return view_proj;
  }
};

struct SceneData {
  static constexpr int c_simple_mat_max = 256;
  static constexpr int c_simple_model_max = 1024;

  // holding for default material
  BufferHandle objects_cb;
  BufferHandle materials_cb;
  using GeometryData = GeometryBufferHandles;
  struct MaterialData {
    ShaderArgumentHandle arg;
    Vec4 parset0;
    Vec4 parset1;
    size_t cb_index;
    Vec4& albedo() { return parset0; }
    real_t& smoothness() { return parset1.x; }
    real_t& metalness() { return parset1.y; }
    real_t& emissive() { return parset1.z; }
  };
  struct ModelData {
    Geometry::ID geo_id;
    Material::ID mat_id;
    // TODO support root cb offseting
    ShaderArgumentHandle raster_arg;
    ShaderArgumentHandle raytrace_shadertable_arg;
    size_t cb_index;
    size_t tlas_instance_id;
    Mat4 trans;
  };
  Hashmap<Geometry::ID, GeometryData> geometries;
  Hashmap<Material::ID, MaterialData> materials;
  Hashmap<Model::ID, ModelData> models;

  // Accelration structure and shader table
  bool tlas_or_shadertable_dirty;
  BufferHandle tlas;
  BufferHandle multibounce_shadetable;

  // Direct lighting const buffer
  BufferHandle punctual_lights;
  std::vector<ShaderArgumentHandle> punctual_light_arg_cache;
  BufferHandle area_lights;
  std::vector<ShaderArgumentHandle> area_light_arg_cache;
};

struct HybridData {
  ShaderArgumentHandle rt_gi_pass_arg;
  ShaderArgumentHandle rt_shdow_pass_arg;
  ShaderArgumentHandle puntual_lighting_arg;
  ShaderArgumentHandle area_lighting_arg;
};

} // namespace hybrid

using namespace hybrid;

struct HybridGPassShaderDesc : RasterizationShaderMetaInfo {
  HybridGPassShaderDesc() {
    ShaderParameter space0 {};
    space0.const_buffers = {ConstantBuffer()};
    ShaderParameter space1 {};
    space1.const_buffers = {ConstantBuffer()};
    ShaderParameter space2 {};
    space2.const_buffers = {ConstantBuffer()};

    RenderTargetDesc rt_normal {ResourceFormat::R32G32B32A32Float};
    RenderTargetDesc rt_albedo {ResourceFormat::R8G8B8A8Unorm};
    RenderTargetDesc rt_emissive {ResourceFormat::R32G32B32A32Float};
    render_target_descs = {rt_normal, rt_albedo, rt_emissive};

    signature.param_table = {space0, space1};
  }
};

struct HybridRaytracingShaderDesc : RaytracingShaderMetaInfo {
  HybridRaytracingShaderDesc() {
    ShaderParameter space0 {};
    space0.const_buffers = {ConstantBuffer()}; // Per-render CN
    space0.shader_resources = {ShaderResource(), ShaderResource(), ShaderResource(),
      ShaderResource(), ShaderResource()}; // TLAS ans G-Buffer
    space0.unordered_accesses = {
      UnorderedAccess(), // GI standalone output buffer
      UnorderedAccess(), // also output to final shading buffer, for now
    };
    global_signature.param_table = {space0};

    ShaderParameter space1 {};
    space1.shader_resources = {ShaderResource(), ShaderResource()}; // indices and vertices
    space1.const_buffers = {ConstantBuffer()};                      // material
    hitgroup_signature.param_table = {{}, space1};

    hitgroup_name = L"hit_group0";
    raygen_name = L"raygen_shader";
    // raygen_name = L"raygen_plain_color";
    closest_hit_name = L"closest_hit_shader";
    miss_name = L"miss_shader";
  }
};

struct HybridBaseShadingShaderDesc : ComputeShaderMetaInfo {
  HybridBaseShadingShaderDesc() {
    ShaderParameter space1 {};
    space1.unordered_accesses = {
      UnorderedAccess(), // output color
    };
    signature.param_table = {{}, space1};
  }
};

struct HybridDirectLightingShaderDesc : ComputeShaderMetaInfo {
  HybridDirectLightingShaderDesc() {
    ShaderParameter space0 {};
    space0.const_buffers = {
      ConstantBuffer(), // light data
    };
    ShaderParameter space1 {};
    space1.shader_resources = {
      ShaderResource(), // depth
      ShaderResource(), // gbuffers
      ShaderResource(),
      ShaderResource(),
    };
    space1.unordered_accesses = {
      UnorderedAccess(), // output color
    };
    space1.const_buffers = {
      ConstantBuffer(), // per-render data
    };
    signature.param_table = {space0, space1};
  }
};

struct HybridDirectAreaLightingShaderDesc : HybridDirectLightingShaderDesc {
  HybridDirectAreaLightingShaderDesc() {}
};

struct BlitShaderDesc : RasterizationShaderMetaInfo {
  BlitShaderDesc() {
    ShaderParameter space0 {};
    space0.shader_resources = {ShaderResource()};
    space0.static_samplers = {StaticSampler()};
    signature.param_table = {space0};

    this->is_depth_stencil_disabled = true;

    RenderTargetDesc dest {ResourceFormat::R8G8B8A8Unorm};
    this->render_target_descs = {dest};
  }
};

struct TaaShaderDesc : ComputeShaderMetaInfo {
  TaaShaderDesc() {
    ShaderParameter space0;
    space0.const_buffers = {ConstantBuffer()};
    space0.shader_resources = {ShaderResource(), ShaderResource()};     // input and history
    space0.unordered_accesses = {UnorderedAccess(), UnorderedAccess()}; // two outputs
    signature.param_table = {space0};
  }
};

HybridPipeline::HybridPipeline(std::weak_ptr<Renderer> renderer, SystemWindowID wnd_id,
  std::weak_ptr<Scene> scene, std::weak_ptr<Camera> camera)
    : m_renderer(renderer),
      m_scene(scene),
      m_camera(camera),
      m_enable_multibounce(true),
      m_enabled_accumulated_rtrt(true),
      m_area_shadow_ssp_per_light(4),
      m_sto_shadow_pass(renderer) {
  std::shared_ptr<Renderer> r = renderer.lock();

  // Init pipeline
  {
    m_gpass_shader
      = r->create_shader(L"CoreData/shader/hybrid_gpass.hlsl", HybridGPassShaderDesc());
  }

  {
    m_base_shading_shader = r->create_shader(
      L"CoreData/shader/hybrid_base_shading.hlsl", HybridBaseShadingShaderDesc());
    m_punctual_lighting_shader = r->create_shader(
      L"CoreData/shader/hybrid_direct_lighting.hlsl", HybridDirectLightingShaderDesc());
    m_area_lighting_shader = r->create_shader(
      L"CoreData/shader/hybrid_direct_area_lighting.hlsl", HybridDirectAreaLightingShaderDesc());
  }

  {
    m_multibounce_shader
      = r->create_shader(L"CoreData/shader/hybrid_multibounce.hlsl", HybridRaytracingShaderDesc());
  }

  {
    m_blit_shader = r->create_shader(L"CoreData/shader/blit.hlsl", BlitShaderDesc());
    m_taa_shader = r->create_shader(L"CoreData/shader/taa.hlsl", TaaShaderDesc());
  }

  {
    ConstBufferLayout lo = {
      ShaderDataType::Float4,   // screen size
      ShaderDataType::Float4x4, // := camera proj_inv
      ShaderDataType::Float4x4, // world_to_proj := camera view_proj
      ShaderDataType::Float4x4, // proj_to_world := camera view_proj inverse
      ShaderDataType::Float4,   // camera pos
      ShaderDataType::Float4,   // frame id
    };
    m_per_render_buffer = r->create_const_buffer(lo, 1, L"PerRenderCB");
  }

  // Init viewport data
  m_viewport_data = std::make_unique<ViewportData>();
  { m_viewport_data->wnd_id = wnd_id; }

  // Init scene data
  m_scene_data = std::make_unique<SceneData>();
  {
    std::shared_ptr<Scene> scene = m_scene.lock();
    SceneData& proxy = *m_scene_data;
    SceneData& scene_data = *m_scene_data;

    // Initialize Geometries
    {
      // nothing ...
    } // Initialize Materials
    {
      ConstBufferLayout simple_mat_lo = {
        ShaderDataType::Float4, // albedo
        ShaderDataType::Float4, // metalness and smoothness
      };
      scene_data.materials_cb
        = r->create_const_buffer(simple_mat_lo, SceneData::c_simple_mat_max, L"Simple Material CB");
    }

    // Initialize Geometry, Material, and assigning raster/ray-tracing shader arguments
    {
      ConstBufferLayout cb_lo = {
        ShaderDataType::Float4x4, // WVP
        ShaderDataType::Float4x4, // World
      };
      proxy.objects_cb
        = r->create_const_buffer(cb_lo, SceneData::c_simple_model_max, L"Simple Model CB");
      proxy.tlas_or_shadertable_dirty = true;
    }

    // Allocate analitic light buffer
    {
      ConstBufferLayout lo {};
      lo[0] = ShaderDataType::Float4; // pos_dir
      lo[1] = ShaderDataType::Float4; // color
      proxy.punctual_lights = r->create_const_buffer(lo, 128, L"Punctual Lights Buffer");
    }
    {
      ConstBufferLayout lo {};
      lo[0] = ShaderDataType::Float4; // shaper
      lo[1] = ShaderDataType::Float4; // color
      lo[2] = ShaderDataType::Float4; // stochastic data
      proxy.area_lights = r->create_const_buffer(lo, 128, L"Area Lights Buffer");
    }
  }

  // Init hybrid data
  m_hybrid_data = std::make_unique<HybridData>();
}

// even though this is a default destructor, we place it here to allow std::unique_ptr destructor
// generation.
HybridPipeline::~HybridPipeline() = default;

void HybridPipeline::on_create_geometry(const Geometry& geo) {
  auto r = m_renderer.lock();
  auto scene = m_scene.lock();

  GeometryDesc desc {};
  desc.flags.dynamic = false;
  desc.flags.include_blas = true;
  GeometryBufferHandles buffers = r->create_geometry(geo, desc);

  REI_ASSERT(!m_scene_data->geometries.has(geo.id()));
  m_scene_data->geometries.insert({geo.id(), buffers});
}

void HybridPipeline::on_create_material(const Material& mat) {
  auto r = m_renderer.lock();

  // NOTE only support simple material, currently
  // NOTE only allocate cb linear
  // TODO handle material deostroy

  const size_t index = m_scene_data->materials.size();
  if (REI_ERRORIF(index >= SceneData::c_simple_mat_max, "Too much simple material")) return;

  SceneData::MaterialData data {};
  data.cb_index = index;
  {
    ShaderArgumentValue v {};
    v.const_buffers
      = {m_scene_data->materials_cb}; // currenlty, materials are just somt value config
    v.const_buffer_offsets = {index};
    ShaderArgumentHandle mat_arg = r->create_shader_argument(v);
    REI_ASSERT(mat_arg);
    data.arg = mat_arg;
  }

  m_scene_data->materials.insert({mat.id(), data});
}

void HybridPipeline::on_create_model(const Model& model) {
  auto r = m_renderer.lock();

  // NOTO tlas ID is allocated lineaylly
  // TODO handle tlas allocation

  if (REI_ERRORIF(model.get_geometry() == nullptr, "Model has empty geometry")) return;
  if (REI_ERRORIF(model.get_material() == nullptr, "Model has empty material")) return;
  const Geometry::ID geo_id = model.get_geometry()->id();
  const Material::ID mat_id = model.get_material()->id();

  auto geo_ptr = m_scene_data->geometries.try_get(geo_id);
  if (REI_ERRORIF(geo_ptr == nullptr, "Fail to find geometry data")) return;
  auto& geo = *geo_ptr;

  auto mat_ptr = m_scene_data->materials.try_get(mat_id);
  if (REI_ERRORIF(mat_ptr == nullptr, "Fail to find material data")) return;
  auto& mat = *mat_ptr;

  const size_t cb_index = m_scene_data->models.size();

  ShaderArgumentHandle raster_arg;
  {
    ShaderArgumentValue raster_arg_value = {};
    raster_arg_value.const_buffers = {m_scene_data->objects_cb};
    raster_arg_value.const_buffer_offsets = {cb_index};
    raster_arg = r->create_shader_argument(raster_arg_value);
  }
  ShaderArgumentHandle raytrace_arg;
  {
    ShaderArgumentValue raytrace_arg_value;
    raytrace_arg_value.const_buffers = {m_scene_data->materials_cb};
    raytrace_arg_value.const_buffer_offsets = {mat.cb_index};
    raytrace_arg_value.shader_resources = {geo.index_buffer, geo.vertex_buffer};
    raytrace_arg = r->create_shader_argument(raytrace_arg_value);
  }
  size_t tlas_instance_id = cb_index;
  Mat4 init_trans = model.get_transform();
  REI_ASSERT(raster_arg != c_empty_handle);
  REI_ASSERT(raytrace_arg != c_empty_handle);
  SceneData::ModelData data {
    geo_id, mat_id, raster_arg, raytrace_arg, cb_index, tlas_instance_id, init_trans};

  REI_ASSERT(!m_scene_data->models.has(model.id()));
  m_scene_data->models.insert({model.id(), std::move(data)});
}

void HybridPipeline::render(size_t width, size_t height) {
  std::shared_ptr<Renderer> renderer_ptr = m_renderer.lock();
  std::shared_ptr<Camera> camera_ptr = m_camera.lock();

  // alias
  Renderer& renderer = *renderer_ptr;
  const auto cmd_list = renderer.prepare();
  Camera& camera = *camera_ptr;
  ViewportData& viewport = *m_viewport_data;
  SceneData& scene = *m_scene_data;
  HybridData& hybrid = *m_hybrid_data;

  // update scene and view port data
  update_scene();
  update_viewport(width, height);
  update_hybrid();

  // render info
  Mat4 view_proj = viewport.get_view_proj(viewport.view_proj_dirty && m_enable_jittering);
  double taa_blend_factor
    = viewport.view_proj_dirty ? 1.0 : (m_enabled_accumulated_rtrt ? 0.01 : 0.5);

  // Update pre-render const buffer (aka per-frame CB)
  {
    Vec4 screen = Vec4(viewport.width, viewport.height, 0, 0);
    cmd_list->update_const_buffer(m_per_render_buffer, 0, 0, screen);
    cmd_list->update_const_buffer(m_per_render_buffer, 0, 1, viewport.proj_inv);
    cmd_list->update_const_buffer(m_per_render_buffer, 0, 2, viewport.view_proj);
    cmd_list->update_const_buffer(m_per_render_buffer, 0, 3, viewport.view_proj_inv);
    cmd_list->update_const_buffer(m_per_render_buffer, 0, 4, viewport.cam_pos);
    Vec4 render_info = Vec4(viewport.frame_id, -1, -1, -1);
    cmd_list->update_const_buffer(m_per_render_buffer, 0, 5, render_info);
  }

  // Update material buffer
  {
    for (auto& pair : scene.materials) {
      auto& mat = pair.second;
      renderer.update_const_buffer(scene.materials_cb, mat.cb_index, 0, mat.parset0);
      renderer.update_const_buffer(scene.materials_cb, mat.cb_index, 1, mat.parset1);
    }
  }

  //-------
  // Pass: Create G-Buffer

  // Update per-object const buffer
  {
    for (auto& pair : scene.models) {
      auto& model = pair.second;
      size_t index = model.cb_index;
      Mat4& m = model.trans;
      Mat4 mvp = view_proj * m;
      cmd_list->update_const_buffer(scene.objects_cb, index, 0, mvp);
      cmd_list->update_const_buffer(scene.objects_cb, index, 1, m);
    }
  }

  // Draw to G-Buffer
  cmd_list->transition(viewport.gbuffer0, ResourceState::RenderTarget);
  cmd_list->transition(viewport.gbuffer1, ResourceState::RenderTarget);
  cmd_list->transition(viewport.gbuffer2, ResourceState::RenderTarget);
  cmd_list->transition(viewport.depth_stencil_buffer, ResourceState::DeptpWrite);
  RenderPassCommand gpass = {};
  {
    gpass.render_targets = {viewport.gbuffer0, viewport.gbuffer1, viewport.gbuffer2};
    gpass.depth_stencil = viewport.depth_stencil_buffer;
    gpass.clear_ds = true;
    gpass.clear_rt = true;
    gpass.viewport = RenderViewaport::full(viewport.width, viewport.height);
    gpass.area = RenderArea::full(viewport.width, viewport.height);
  }
  cmd_list->begin_render_pass(gpass);
  {
    DrawCommand draw_cmd = {};
    draw_cmd.shader = m_gpass_shader;
    for (auto pair : scene.models) {
      auto& model = pair.second;
      auto geo_ptr = scene.geometries.try_get(model.geo_id);
      auto mat_ptr = scene.materials.try_get(model.mat_id);
      REI_ASSERT(geo_ptr && mat_ptr);
      draw_cmd.index_buffer = geo_ptr->index_buffer;
      draw_cmd.vertex_buffer = geo_ptr->vertex_buffer;
      draw_cmd.arguments = {model.raster_arg, mat_ptr->arg};
      cmd_list->draw(draw_cmd);
    }
  }
  cmd_list->end_render_pass();

  //--- End G-Buffer pass

  //-------
  // Pass: Deferred direct lightings, both punctual and area lights

  // Base Shading
  cmd_list->transition(viewport.deferred_shading_output, ResourceState::UnorderedAccess);
  {
    DispatchCommand dispatch {};
    dispatch.compute_shader = m_base_shading_shader;
    dispatch.arguments = {viewport.base_shading_inout_arg};
    dispatch.dispatch_x = viewport.width / 8;
    dispatch.dispatch_y = viewport.height / 8;
    dispatch.dispatch_z = 1;
    cmd_list->dispatch(dispatch);
  }

  cmd_list->barrier(viewport.deferred_shading_output);

  // Punctual lights
  // TODO: same texture input for multibounce GI
  cmd_list->transition(viewport.gbuffer0, ResourceState::ComputeShaderResource);
  cmd_list->transition(viewport.gbuffer1, ResourceState::ComputeShaderResource);
  cmd_list->transition(viewport.gbuffer2, ResourceState::ComputeShaderResource);
  cmd_list->transition(viewport.depth_stencil_buffer, ResourceState::ComputeShaderResource);
  cmd_list->transition(viewport.deferred_shading_output, ResourceState::UnorderedAccess);
  // currently both taa and direct-lighting write to the same UA buffer
  cmd_list->barrier(viewport.deferred_shading_output);
  static int light_indices[] = {0, 1};
  static Color light_colors[] = {Colors::white * 1.3, Colors::white * 1};
  static Vec4 light_pos_dirs[] = {{Vec3(1, 2, 1).normalized(), 0}, {0, 2.5, 0, 1}};
  const int light_count = ARRAYSIZE(light_indices);
  for (int i = 0; i < light_count; i++) {
    int light_index = light_indices[i];
    Vec4 light_color = Vec4(light_colors[i]);
    Vec4 light_pos_dir = light_pos_dirs[i];

    // update light data
    cmd_list->update_const_buffer(scene.punctual_lights, light_index, 0, light_pos_dir);
    cmd_list->update_const_buffer(scene.punctual_lights, light_index, 1, light_color);

    // dispatch
    DispatchCommand dispatch {};
    dispatch.compute_shader = m_punctual_lighting_shader;
    dispatch.arguments = {fetch_direct_punctual_lighting_arg(renderer, scene, light_index),
      viewport.direct_lighting_inout_arg};
    dispatch.dispatch_x = viewport.width / 8;
    dispatch.dispatch_y = viewport.height / 8;
    dispatch.dispatch_z = 1;
    cmd_list->dispatch(dispatch);
  }

  // Area lights
  // TODO: same texture input for multibounce GI
  cmd_list->transition(viewport.gbuffer0, ResourceState::ComputeShaderResource);
  cmd_list->transition(viewport.gbuffer1, ResourceState::ComputeShaderResource);
  cmd_list->transition(viewport.gbuffer2, ResourceState::ComputeShaderResource);
  cmd_list->transition(viewport.depth_stencil_buffer, ResourceState::ComputeShaderResource);
  cmd_list->transition(viewport.area_light.unshadowed, ResourceState::UnorderedAccess);
  cmd_list->clear_texture(viewport.area_light.unshadowed, {0, 0, 0, 0},
    RenderArea::full(viewport.width, viewport.height));
  cmd_list->barrier(viewport.area_light.unshadowed);
  static std::array<int, 2> area_light_indices = {0, 1};
  static std::array<Vec4, 2> area_light_shapes = {
    Vec4(-100, 50, 60, 1),
    Vec4(-5, 4, -3, .5),
  };
  static std::array<Color, 2> area_light_colors {
    Colors::white * 30000,
    Colors::white * 400,
  };
  for (int i = 0; i < area_light_indices.size(); i++) {
    int light_index = area_light_indices[i];
    Vec4 light_color = Vec4(area_light_colors[i]);
    Vec4 light_shape = area_light_shapes[i];

    // update light data
    cmd_list->update_const_buffer(scene.area_lights, light_index, 0, light_shape);
    cmd_list->update_const_buffer(scene.area_lights, light_index, 1, light_color);

    // dispatch
    DispatchCommand dispatch {};
    dispatch.compute_shader = m_area_lighting_shader;
    dispatch.arguments = {fetch_direct_area_lighting_arg(renderer, scene, light_index),
      viewport.area_light.unshadowed_pass_arg};
    dispatch.dispatch_x = viewport.width / 8;
    dispatch.dispatch_y = viewport.height / 8;
    dispatch.dispatch_z = 1;
    cmd_list->dispatch(dispatch);
  }

  // --- End Deferred directy lighting

  //-------
  // Pass: Raytraced multi-bounced GI
  if (m_enable_multibounce) {
    // TODO move this to a seperated command queue

    // Update shader Tables
    {
      UpdateShaderTable desc = UpdateShaderTable::hitgroup();
      desc.shader = m_multibounce_shader;
      desc.shader_table = scene.multibounce_shadetable;
      for (auto& pair : scene.models) {
        auto& m = pair.second;
        desc.index = m.tlas_instance_id;
        desc.arguments = {m.raytrace_shadertable_arg};
        cmd_list->update_shader_table(desc);
      }
    }

    // Trace
    cmd_list->transition(viewport.gbuffer0, ResourceState::ComputeShaderResource);
    cmd_list->transition(viewport.gbuffer1, ResourceState::ComputeShaderResource);
    cmd_list->transition(viewport.gbuffer2, ResourceState::ComputeShaderResource);
    cmd_list->transition(viewport.depth_stencil_buffer, ResourceState::ComputeShaderResource);
    cmd_list->transition(viewport.raytracing_output_buffer, ResourceState::UnorderedAccess);
    {
      RaytraceCommand cmd {};
      cmd.raytrace_shader = m_multibounce_shader;
      cmd.arguments = {hybrid.rt_gi_pass_arg};
      cmd.shader_table = scene.multibounce_shadetable;
      cmd.width = viewport.width;
      cmd.height = viewport.height;
      cmd_list->raytrace(cmd);
    }
  }
  //--- End multi-bounce GI pass

  // -------
  // Pass: Stochastic Shadow
  m_sto_shadow_pass.run(
    viewport.area_light.unshadowed, viewport.frame_id, m_area_shadow_ssp_per_light);
  //--- End Stochastic Shadow

  // -------
  // Pass: TAA on final shading result
  {
    Vec4 parset0 {real_t(viewport.frame_id), real_t(taa_blend_factor), -1, -1};
    cmd_list->update_const_buffer(viewport.taa_cb, 0, 0, parset0);
  }
  cmd_list->transition(viewport.taa_curr_input(), ResourceState::ComputeShaderResource);
  cmd_list->transition(viewport.taa_curr_output(), ResourceState::UnorderedAccess);
  cmd_list->transition(viewport.deferred_shading_output, ResourceState::UnorderedAccess);
  {
    DispatchCommand dispatch {};
    dispatch.compute_shader = m_taa_shader;
    dispatch.arguments = {viewport.taa_curr_arg()};
    dispatch.dispatch_x = viewport.width / 8;
    dispatch.dispatch_y = viewport.height / 8;
    dispatch.dispatch_z = 1;
    cmd_list->dispatch(dispatch);
  }

  // --- End TAA pass

  // -------
  // Pass: Blit Post-processing reults to render target

  BufferHandle render_target = renderer.fetch_swapchain_render_target_buffer(viewport.swapchain);
  cmd_list->transition(render_target, ResourceState::RenderTarget);
  cmd_list->transition(viewport.deferred_shading_output, ResourceState::PixelShaderResource);
  {
    RenderPassCommand render_pass {};
    render_pass.render_targets = {render_target};
    render_pass.depth_stencil = c_empty_handle;
    render_pass.clear_ds = false;
    render_pass.clear_rt = true;
    render_pass.viewport = RenderViewaport::full(viewport.width, viewport.height);
    render_pass.area = RenderArea::full(viewport.width, viewport.height);
    cmd_list->begin_render_pass(render_pass);

    DrawCommand draw {};
    draw.arguments = {viewport.blit_for_present};
    draw.shader = m_blit_shader;
    cmd_list->draw(draw);

    cmd_list->end_render_pass();
  }

  // --- End blitting

  // some debug blits
#if !DEBUG
  if (false)
#endif
  {
    const size_t blit_height = viewport.height / 8;
    const size_t blit_width = blit_height * viewport.width / viewport.height;
    int debug_blit_count = 0;
    const RenderViewaport full_vp = RenderViewaport::full(viewport.width, viewport.height);
    const RenderArea full_area = RenderArea::full(viewport.width, viewport.height);
    auto debug_blit_pass
      = [&](BufferHandle texture, ShaderArgumentHandle blit_arg, bool full = false) {
          cmd_list->transition(texture, ResourceState::PixelShaderResource);
          RenderPassCommand render_pass {};
          render_pass.render_targets = {render_target};
          render_pass.depth_stencil = c_empty_handle;
          render_pass.clear_ds = false;
          render_pass.clear_rt = false;
          if (full) {
            render_pass.viewport = full_vp;
            render_pass.area = full_area;
          } else {
            render_pass.viewport = full_vp.shrink_to_upper_left(
              blit_width, blit_height, 0, blit_height * debug_blit_count);
            render_pass.area = full_area.shrink_to_upper_left(
              blit_width, blit_height, 0, blit_height * debug_blit_count);
          }
          cmd_list->begin_render_pass(render_pass);

          DrawCommand draw {};
          draw.arguments = {blit_arg};
          draw.shader = m_blit_shader;
          cmd_list->draw(draw);

          cmd_list->end_render_pass();

          debug_blit_count++;
        };

    debug_blit_pass(viewport.area_light.unshadowed, viewport.area_light.blit_unshadowed);
    debug_blit_pass(viewport.raytracing_output_buffer, viewport.blit_raytracing_output);
  }

  // Update frame-counting in the end
  viewport.advance_frame();

  cmd_list->transition(render_target, ResourceState::Present);
  cmd_list->present(viewport.swapchain, false);
}

void HybridPipeline::update_scene() {
  std::shared_ptr<Scene> scene = m_scene.lock();
  SceneData& proxy = *m_scene_data;
  SceneData& scene_data = *m_scene_data;

  auto r = m_renderer.lock();

  Hashmap<Material::ID, Material*> curr_materials;

  // Fetch model transform
  // TODO mark ditry for geometry instances
  for (ModelPtr& m : scene->get_models()) {
    Model::ID id = m->id();
    auto* data = proxy.models.try_get(id);
    REI_ASSERT(data);
    data->trans = m->get_transform();
    // collect materials
    auto& mat = m->get_material();
    if (mat) { curr_materials.insert(mat->id(), mat.get()); }
  }

  // Fetch current material data
  for (auto& pair : curr_materials) {
    Material::ID id = pair.first;
    Material* mat = pair.second;
    REI_ASSERT(mat);
    auto* data = proxy.materials.try_get(id);
    REI_ASSERT(data);

    data->albedo() = mat->get<Color>(L"albedo").value_or(Colors::magenta).to_vec4();
    data->smoothness() = mat->get<double>(L"smoothness").value_or(0);
    data->metalness() = mat->get<double>(L"metalness").value_or(0);
    data->emissive() = mat->get<double>(L"emissive").value_or(0);
  }

  curr_materials.clear();

  // Re-build Acceleration structure and shader table
  if (scene_data.tlas_or_shadertable_dirty) {
    if (proxy.tlas != c_empty_handle)
      REI_ERROR("TODO destroy old tlas, or do tlas update instead of creation");
    if (proxy.multibounce_shadetable != c_empty_handle)
      REI_ERROR("TODO destroy old shader-table, or do shader-table update instead of creation");

    RaytraceSceneDesc desc {};
    for (auto& kv : scene_data.models) {
      auto& m = kv.second;
      auto geo_ptr = scene_data.geometries.try_get(m.geo_id);
      REI_ASSERT(geo_ptr);
      desc.instance_id.push_back(m.tlas_instance_id);
      desc.blas_buffer.push_back(geo_ptr->blas_buffer);
      desc.transform.push_back(m.trans);
    }
    proxy.tlas = r->create_raytracing_accel_struct(desc);
    proxy.multibounce_shadetable = r->create_shader_table(*scene, m_multibounce_shader);

    scene_data.tlas_or_shadertable_dirty = false;
  }
}

void HybridPipeline::update_viewport(size_t width, size_t height) {
  // ignore case when minimixed
  if (width == 0 || height == 0) return;

  REI_ASSERT(width > 0 && height > 0);
  std::shared_ptr<Renderer> renderer_ptr = m_renderer.lock();
  std::shared_ptr<Camera> camera_ptr = m_camera.lock();

  // alias
  Renderer* r = renderer_ptr.get();
  Camera& camera = *camera_ptr;
  ViewportData& proxy = *m_viewport_data;

  // check transforms
  {
    Mat4 new_vp = r->is_depth_range_01() ? camera.view_proj_halfz() : camera.view_proj();
    Mat4 diff_mat = new_vp - proxy.view_proj;
    if (!m_enabled_accumulated_rtrt || diff_mat.norm2() > 0) {
      proxy.cam_pos = {camera.position(), 1};
      proxy.proj = camera.project();
      proxy.proj_inv = proxy.proj.inv();
      proxy.view_proj = new_vp;
      proxy.view_proj_inv = new_vp.inv();
      proxy.view_proj_dirty = true;
    }
  }

  const bool size_changed = (width != proxy.width) || (height != proxy.height);
  const bool uninitialized = proxy.width == 0;
  if (size_changed) {
    // TODO release old resources
    if (!uninitialized) REI_WARNING("previously create viewport resources is not released!");
  }

  proxy.width = width;
  proxy.height = height;

  const bool create_resources = uninitialized || size_changed;
  if (create_resources) {
    proxy.swapchain = r->create_swapchain(proxy.wnd_id, width, height, 2);

    proxy.depth_stencil_buffer = r->create_texture_2d(
      TextureDesc::depth_stencil(width, height), ResourceState::DeptpWrite, L"Depth Stencil");
    proxy.gbuffer0 = r->create_texture_2d(
      TextureDesc::render_target(width, height, ResourceFormat::R32G32B32A32Float),
      ResourceState::RenderTarget, L"Normal Buffer");
    proxy.gbuffer1 = r->create_texture_2d(
      TextureDesc::render_target(width, height, ResourceFormat::R8G8B8A8Unorm),
      ResourceState::RenderTarget, L"Albedo Buffer");
    proxy.gbuffer2 = r->create_texture_2d(
      TextureDesc::render_target(width, height, ResourceFormat::R32G32B32A32Float),
      ResourceState::RenderTarget, L"Emissive Buffer");

    proxy.raytracing_output_buffer = r->create_unordered_access_buffer_2d(
      width, height, ResourceFormat::R32G32B32A32Float, L"Raytracing Output Buffer");

    proxy.deferred_shading_output = r->create_texture_2d(
      TextureDesc::unorder_access(width, height, ResourceFormat::R32G32B32A32Float),
      ResourceState::UnorderedAccess, L"Deferred Shading Output");

    proxy.area_light.unshadowed = r->create_texture_2d(
      TextureDesc::unorder_access(width, height, ResourceFormat::R32G32B32A32Float),
      ResourceState::UnorderedAccess, L"Area Light Unshadowed");

    // helper routine
    auto make_blit_arg = [&](BufferHandle source) {
      ShaderArgumentValue val {};
      val.shader_resources = {source};
      return r->create_shader_argument(val);
    };

    // Base shading pass argument
    {
      ShaderArgumentValue val {};
      val.unordered_accesses = {proxy.deferred_shading_output};
      proxy.base_shading_inout_arg = r->create_shader_argument(val);
      REI_ASSERT(proxy.base_shading_inout_arg);
    }

    // punctual light argumnets
    {
      ShaderArgumentValue val {};
      val.shader_resources
        = {proxy.depth_stencil_buffer, proxy.gbuffer0, proxy.gbuffer1, proxy.gbuffer2};
      val.unordered_accesses = {proxy.deferred_shading_output};
      val.const_buffers = {m_per_render_buffer};
      val.const_buffer_offsets
        = {0}; // FIXME current all viewport using the same part of per-render buffer
      proxy.direct_lighting_inout_arg = r->create_shader_argument(val);
      REI_ASSERT(proxy.direct_lighting_inout_arg);
    }

    // area light arguments
    {
      ShaderArgumentValue val {};
      val.shader_resources
        = {proxy.depth_stencil_buffer, proxy.gbuffer0, proxy.gbuffer1, proxy.gbuffer2};
      val.unordered_accesses = {proxy.area_light.unshadowed};
      val.const_buffers = {m_per_render_buffer};
      val.const_buffer_offsets
        = {0}; // FIXME current all viewport using the same part of per-render buffer
      proxy.area_light.unshadowed_pass_arg = r->create_shader_argument(val);
      REI_ASSERT(proxy.area_light.unshadowed_pass_arg);
    }

    // Multibounce tracing arguments
    {
      // debug blit
      proxy.blit_raytracing_output = make_blit_arg(proxy.raytracing_output_buffer);
    }

    // TAA arguments
    {
      ConstBufferLayout lo {};
      lo[0] = ShaderDataType::Float4;
      proxy.taa_cb = r->create_const_buffer(lo, 1, L"TAA CB");
    }
    {
      static const wchar_t* taa_names[2] = {L"TAA_Buffer[0]", L"TAA_Buffer[1]"};
      auto make_taa_buffer = [&](int idx) {
        return r->create_texture_2d(
          TextureDesc::unorder_access(width, height, ResourceFormat::R32G32B32A32Float),
          ResourceState::UnorderedAccess, taa_names[idx]);
      };
      for (int i = 0; i < 2; i++)
        proxy.taa_buffer[i] = make_taa_buffer(i);
    }
    {
      auto make_taa_argument = [&](int input, int output) {
        ShaderArgumentValue v {};
        // TODO we feed deferred shading raw result to TAA, but better is to do a tonemapping first
        v.const_buffers = {proxy.taa_cb};
        v.const_buffer_offsets = {0};
        v.shader_resources = {proxy.taa_buffer[input], proxy.deferred_shading_output};
        v.unordered_accesses = {proxy.taa_buffer[output], proxy.deferred_shading_output};
        return r->create_shader_argument(v);
      };
      for (int i = 0; i < 2; i++)
        proxy.taa_argument[i] = make_taa_argument(i, (i + 1) % 2);
    }

    // final blit
    proxy.blit_for_present = make_blit_arg(proxy.deferred_shading_output);

    // mark dirty
    proxy.gbuffer_dirty = true;
  }
}

void HybridPipeline::update_hybrid() {
  std::shared_ptr<Renderer> renderer_ptr = m_renderer.lock();
  Renderer& renderer = *renderer_ptr;
  SceneData& scene = *m_scene_data;
  ViewportData& viewport = *m_viewport_data;
  HybridData& hybrid = *m_hybrid_data;

  // Raytracing GI pass argument
  if (viewport.gbuffer_dirty || !hybrid.rt_gi_pass_arg) {
    if (hybrid.rt_gi_pass_arg) REI_WARNING("TODO release old shader arguments");

    ShaderArgumentValue val {};
    val.const_buffers = {m_per_render_buffer};
    val.const_buffer_offsets = {0};
    val.shader_resources = {
      scene.tlas,                    // t0 TLAS
      viewport.depth_stencil_buffer, // t1 G-buffer depth
      viewport.gbuffer0,             // t2 G-Buffer::normal
      viewport.gbuffer1,             // t3 G-Buffer::albedo
      viewport.gbuffer2,             // t4 G-Buffer::emissive
    };
    val.unordered_accesses = {
      viewport.raytracing_output_buffer,
      viewport.deferred_shading_output,
    };
    ShaderArgumentHandle arg = renderer.create_shader_argument(val);
    REI_ASSERT(arg);
    m_hybrid_data->rt_gi_pass_arg = arg;
  }

#if SSAL
  // Raytracing shadow pass argument
  if (viewport.gbuffer_dirty || !hybrid.rt_shdow_pass_arg) {
    if (hybrid.rt_shdow_pass_arg) REI_WARNING("TODO release old shader arguments");

    ShaderArgumentValue val {};
    val.const_buffers = {m_per_render_buffer};
    val.const_buffer_offsets = {0};
    val.shader_resources = {
      scene.tlas,                                     // t0 TLAS
      viewport.depth_stencil_buffer,                  // t1 G-buffer depth
      viewport.area_light.stochastic_sample_ray,      // t2 sample ray
      viewport.area_light.stochastic_sample_radiance, // t3 sample radiance
    };
    val.unordered_accesses = {viewport.area_light.stochastic_shadowed};
    ShaderArgumentHandle arg = renderer.create_shader_argument(val);
    REI_ASSERT(arg);
    m_hybrid_data->rt_shdow_pass_arg = arg;
  }
#endif
}

ShaderArgumentHandle HybridPipeline::fetch_direct_punctual_lighting_arg(
  Renderer& renderer, SceneData& scene, int cb_index) {
  REI_ASSERT(cb_index >= 0);
  if (cb_index >= scene.punctual_light_arg_cache.size()) {
    ShaderArgumentValue val {};
    val.const_buffers = {scene.punctual_lights};
    val.const_buffer_offsets = {size_t(cb_index)};
    ShaderArgumentHandle h = renderer.create_shader_argument(val);
    REI_ASSERT(h);
    scene.punctual_light_arg_cache.reserve(cb_index + 1);
    scene.punctual_light_arg_cache.resize(cb_index + 1);
    scene.punctual_light_arg_cache[cb_index] = h;
  }

  return scene.punctual_light_arg_cache[cb_index];
}

ShaderArgumentHandle HybridPipeline::fetch_direct_area_lighting_arg(
  Renderer& renderer, SceneData& scene, int cb_index) {
  REI_ASSERT(cb_index >= 0);
  if (cb_index >= scene.area_light_arg_cache.size()) {
    ShaderArgumentValue val {};
    val.const_buffers = {scene.area_lights};
    val.const_buffer_offsets = {size_t(cb_index)};
    ShaderArgumentHandle h = renderer.create_shader_argument(val);
    REI_ASSERT(h);
    scene.area_light_arg_cache.reserve(cb_index + 1);
    scene.area_light_arg_cache.resize(cb_index + 1);
    scene.area_light_arg_cache[cb_index] = h;
  }

  return scene.area_light_arg_cache[cb_index];
}

} // namespace rei
