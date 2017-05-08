# Cel
Project under construction

## todos

- Issues
  - using conversion operator in Vec4, rather than conversion constructor in
    Vec3 : Vec4 knows more about how to become a Vec3!
  - remove ALPHA channel in the pixels lib: we don't need that. alpha channel
    is useful before putting into color buffer; it have no use in the final
    image !

- Rendering
  - build a three-overlapping triangle test case (test color and z-buffer)
    - support vertex color
    - support depth buffer
  - make a true mesh (use half-edge or winged-triangle?)
  - add model loading, and support general mesh and material
    - skip texture currently
  - add sophisticated culling for better performance
