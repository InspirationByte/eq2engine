# framework/math — math library

`framework/math` is the engine's own vector math library (part of `frameworkLib`). Use it instead
of third-party math libraries in engine code. `math/math_common.h` is included by
`core/core_common.h` and pulls in the core types; utility headers are included explicitly.

| Included by `core_common.h` (via `math_common.h`) | Include explicitly |
|---|---|
| `Vector.h`, `FVector.h`, `Matrix.h`, `Quaternion.h`, `Transform.h`, `Plane.h`, `BoundingBox.h`, `Volume.h`, `Rectangle.h` | `math/Utility.h`, `math/Random.h`, `math/Spline.h`, `math/FixedMath.h`, `math/QuadTree.h` |

Unlike most engine code, math free functions and vector/matrix methods use lowerCamelCase
(`dot`, `normalize`, `lengthSqr`, `transformPoint`); geometry classes (`TAABBox3D`, `Volume`,
`TPlane`, `TAARectangle`) and `Utility.h` functions use PascalCase. Keep to the existing API.

## Scalars and constants — `math_common.h`

- Constants: `F_EPS` (0.00001), `F_INFINITY`, `F_UNDEF` (marker for undefined values), `M_PI_F`,
  `M_PI_D`, `M_PI_HALF_F`, `M_PI_2_F`, `M_RAD2DEG`, `M_DEG2RAD`.
- Conversion: `DEG2RAD(x)`, `RAD2DEG(x)`.
- `SinCos(angle, &s, &c)`, `rsqrtf` (fast inverse square root), `sqr`, `clamp`, `saturate`,
  `fsimilar(a, b, eps)`, `fisNan`, `fisFinite`.
- `RemapVal` / `RemapValClamp` — map a value from range [A,B] to [C,D].
- Powers of two: `isPowerOf2`, `nextPowerOf2`, `getUpperPowerOfTwo`, `getLowerPowerOfTwo`,
  `getClosestPowerOfTwo`.
- `ECubeSide` — cube map faces.

## Vectors — `Vector.h`

| Type | Alias of |
|---|---|
| `Vector2D`, `Vector3D`, `Vector4D` | `TVec2D<float>`, `TVec3D<float>`, `TVec4D<float>` |
| `IVector2D`, `IVector3D`, `IVector4D` | `TVec2D<int>`, ... |
| `FVector2D`, `FVector3D`, `FVector4D` | `TVec*<FReal>` (`FVector.h`; `FReal` is `float` by default) |
| `MColor` | RGBA float color (`r`, `g`, `b`, `a`, or `v` as `Vector4D`) |
| `half` | 16-bit float |

- Components `x`, `y`, `z`, `w`; swizzles `xy()`, `yz()`, `xz()` and more on `TVec4D`.
- Constructors from a single scalar (all components), components, or smaller vector + scalar;
  conversion between element types: `IVector2D iv = Vector2D(...)`.
- Component-wise operators `+ - * /` with vectors and scalars, compound assignment, comparison.
- Free functions (overloaded for 2D/3D/4D):
  - `dot`, `cross` (3D), `length`, `lengthSqr`, `distance`, `distanceSqr`;
  - `normalize`, `normalizeSelf` (returns the old length), `fastNormalize`;
  - `lerp`, `cerp` (cubic), `bezierQuadratic`, `bezierCubic`, `sCurve`, `approachValue`;
  - `clamp`, `sign`, `vecSimilar(u, v, eps)`, `vecIsValid`, `vecIsFinite`;
  - `reflect`, `lineProjection`, `planeDistance`;
  - `rotateVectorAroundAxis`, `rotateVectorXY/XZ/YZ`.
- Constants: `vec2_zero`, `vec3_zero`, `vec4_zero`, `vec*_unit`, `vec3_undef`, `vec3_infinity`,
  axis vectors `vec3_right` (+X), `vec3_up` (+Y), `vec3_forward` (+Z).
- Euler angles: `AngleVectors(angles, forward, right, up)`, `VectorAngles(forward)`,
  `VectorVectors(forward, right, up)`. These take and return **degrees** (legacy convention; most
  other angle APIs — matrix and quaternion rotations, `Transform3D` — use radians); `x` — pitch,
  `y` — yaw, `z` — roll.
- Colors: `color_white`, `color_black`, `color_red`, ...; `toRGBA`, `toBGRA`, `hexToColor3/4`,
  `rgbToRGBE8`, `rgbToRGB9E5`, `lerpRGBAPacked`. `MColor(uint)` takes ABGR byte order,
  `MColor(uint, true)` takes ARGB (`0xAARRGGBB`).

```cpp
Vector3D dir = normalize(target - origin);
const float dist = length(target - origin);
const Vector3D side = cross(dir, vec3_up);

if (vecSimilar(a, b, 0.001f))
	...
```

## Matrices — `Matrix.h`

| Type | Alias of |
|---|---|
| `Matrix2x2`, `Matrix3x3`, `Matrix4x4` | `TMat2<float>`, `TMat3<float>`, `TMat4<float>` |
| `float2x2`, `float3x3`, `float4x4` | same, shader-style names |
| `FMatrix*x*` | `TMat*<FReal>` |

- Row-major storage: `rows[4]` / `r1`…`r4` / `m11`…`m44`; `operator()(row, col)`.
- Column-vector convention: `matrix * vector`; translation lives in the last column
  (`m14`, `m24`, `m34`). Functions with the `Transposed` suffix work with the transposed layout.
- Operators: `*` (matrix × matrix / vector / scalar), `+`, `-`, `!m` — inverse; `inverse(m)`,
  `transpose(m)`, `det(m)`.
- Builders: `identity2/3/4`, `translate(v)`, `scale2/3/4`, `rotateX3/4`, `rotateY3/4`,
  `rotateZ3/4`, `rotateXYZ3/4`, `rotateZXY3/4`, `rotateYZX3/4`, `rotateAxis3`, constructors from
  `Quaternion` or axis + angle. Rotation builders take **radians**.
- Methods: `setTranslation`, `getTranslationComponent`, `getRotationComponent`, `setRotation`.
- Transforming: `transformPoint(p, m)`, `rotateVector(v, m)`, `translateVector(v, m)` and their
  `Transposed` variants.
- Projections: `perspectiveMatrix`, `perspectiveMatrixX`/`Y` (FOV along X/Y), `orthoMatrix`,
  `orthoMatrixR`, `projection2DScreen`, `toD3DProjection`, `toGLProjection`, cube map
  `cubeViewMatrix` / `cubeProjectionMatrixGL` / `D3D`.
- Euler extraction: `EulerMatrixXYZ`, `EulerMatrixZXY`.

```cpp
const Matrix4x4 world = translate(position) * Matrix4x4(rotation) * scale4(s.x, s.y, s.z);
const Vector3D worldPos = transformPoint(localPos, world);
const Matrix4x4 worldToLocal = !world;
```

## Quaternions and transforms — `Quaternion.h`, `Transform.h`

- `Quaternion(w, x, y, z)`, `Quaternion(angle, axis)`, `Quaternion(Matrix3x3)`; identity `qidentity`.
- Operators: `*` (composition), `+`, `-`, scalar `*` and `/`; `!q` and `inverse(q)` — inverse.
- `rotateX/Y/Z`, `rotateXY`, `rotateXYZ`, `rotateZXY` — from angles; `lookAt(forward, up)`.
- `slerp`, `scerp` (spherical cubic), `normalize()`, `renormalize`, `length`,
  `quaternionSimilar`, `axisAngle(q, axis, angle)`, `eulersXYZ`,
  `quaternionToEulers(q, EQuatRotationSequence)`.
- `rotateVector(v, q)` rotates a vector.
- `Transform3D` — translation + orientation + scale; the `eulerAnglesXYZ` constructor argument is in
  **radians** (unlike `AngleVectors` / `VectorAngles`, which use degrees): `forward()`, `right()`, `up()`,
  `toMatrix()`, `!t` / `inverse(t)`, `transformPoint`, `transformPointInverse`, `rotateVector`.

## Geometry

| Type | Header | Purpose |
|---|---|---|
| `Plane` (`TPlane<float>`), `FPlane` | `Plane.h` | `normal` + `offset`; `Distance`, `ClassifyPoint` (`CP_FRONT`/`CP_BACK`/`CP_ONPLANE`), intersections with ray, line, plane(s); `matrix * plane` transforms |
| `BoundingBox` (`TAABBox3D<float>`), `IBoundingBox` | `BoundingBox.h` | AABB: `Reset`, `AddVertex`, `Merge`, `Expand`, `Contains`, `Intersects`, `IntersectsRay`, `IntersectsSphere`, `GetCenter`, `GetSize`, `GetVertex` |
| `AARectangle` (`TAARectangle<float>`), `IAARectangle` | `Rectangle.h` | 2D axis-aligned rectangle (`leftTop`, `rightBottom`): `Contains`, `Intersects`, `GetSize`, splitting helpers, `FlipX/Y` |
| `Volume` | `Volume.h` | Set of 6 planes: `LoadAsFrustum(viewProj)`, `LoadBoundingBox`, `IsPointInside`, `IsSphereInside`, `IsBoxInside`, `IsTriangleInside`, `IsIntersectsRay`; static versions take `ArrayCRef<Plane>` |

A default-constructed `BoundingBox` / `AARectangle` is reset to an invalid (inverted) state; check
`IsValid()` after adding vertices.

```cpp
BoundingBox bbox;
for (const Vector3D& v : vertices)
	bbox.AddVertex(v);

Volume frustum;
frustum.LoadAsFrustum(proj * view);
if (!frustum.IsBoxInside(bbox))
	return;
```

## Utilities — `math/Utility.h`

Implemented in `Utility.cpp`:

- Hashing and snapping: `HashVector2D/3D(v, tolerance)` (spatial hash of quantized vectors),
  `SnapFloat`, `SnapVector`.
- Screen-space: `PointToScreen(point, screen, worldToScreen, screenDims)` (returns `true` when
  the point is **behind** the camera), `ScreenToDirection` (picking ray), `ScreenFrustum` (frustum of
  a screen rectangle).
- Triangles: `UVFromPointOnTriangle`, `PointFromUVOnTriangle`, `IsRayIntersectsTriangle`,
  `PlaneTriangleIntersection`, `ComputeTriangleNormal`, `ComputeTriangleTBN`,
  `ComputeTriangleArea`.
- `IsPointInCone`.
- 2D: `LineIntersectsLine2D`, `LineSegIntersectsLineSeg2D`, `LineSegIntersectsCircle2D`,
  `DistancePointPoly2D`, `ConvexHull2D`, `RectangleRectangleSeparation` (negative when overlapping).
- 3D distances: `DistanceLineLine`, `DistanceLineSegLineSeg` (optional closest-point outputs).
- Angles (degrees unless the name says `Rad`/`PI`): `ConstrainAngle180`, `ConstrainAnglePI`,
  `NormalizeAngles180`, `AngleDiff`, `AngleDiffRad`, `AnglesDiff`, `VecAngleDiff`.
- `PackNormal` / `UnpackNormal` — normal packed into one float.
- `SpringFunction(value, velocity, constant, damping, dt)`, `FT_SquareWave<N>(x)`.

## Other headers

| Header | Contents |
|---|---|
| `Random.h` | `RandomFloat`, `RandomInt`, `RandomSeed` (global); `CUniformRandomStream` and `CPseudoRandomGenerator` for independent, seedable sequences |
| `Spline.h` | `CSpline3D` — timed 3D spline with tangents: position/tangent at time or distance, length; `BezierCubicPoint`, `BezierCubicTangent`, `spline<DIM>` key interpolation |
| `FixedMath.h` | `FReal` scalar type and `FPmath` functions (float by default, `FLOAT_AS_FREAL`) |
| `QuadTree.h` | Barnes-Hut style quad tree over float point data |
