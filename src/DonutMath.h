/*
																		DonutMath 1.0 is a light weight terminal math API 
																			Start of Dev 3/5/2026 | End of dev 5/16/2026
Features:
-Basic math defines | PI, E, TUA, Epsilon, DEG2RAD, RAD2DEG
-Vector2 struct
-Vector3 struct
-Vector4 struct
-3 by 3 Matrix struct
-4 by 4 Matrix struct

Usage:
-Vector2 struct holds a float of x and y values for a 2D grid or 2 number values; Most commonly used for holding a entity's x and y
coordinate value on a 2D grid for movement

-Vector3 struct holds a float of x, y, and z values for a 3D grid or 3 number values; Most commonly used for holding a entity's x,y, and z
coordinate value on a 3D grid for movement; Z value is the coordnate for verticalty on a 3D grid, could be used for jumping or flying movement

-Vector4 struct holds a float of x, y, z and w values for a perspective or look into 4D space giving a 4D POV (Not actully being able to walk or be
in 4D space as we are not 4D creature we are 3D creatures) or 4 number values; Mostly used for formating RGBA, and memory layout for shaders; The 4D space
is a truely confusing subject as we humans are not a 4 dimensional creature, we are 3 dimensional meaning we may go Up, Down, Left and Right. In renderering
we could have a cube rotating inside of a bigger cube (Look to the wiki link, they have a cool image of this)
So we can move on x,y,z position but 4D space adds another position know as w or t which in programing is like looking into 4D space (I recommond doing more
reaserch your self on this as I do not fully understand 4D space as it really is theoretical and I am 16 and still in highschool :). ), while
in physics it is know as t or time. Look to the docs I listed and do your own reaserch for a better understanding.

-Matrix struct holds an array of either 4 colums by 4 rows or 3 colums by 3 rows (in this Matrix struct) in a efficient memory layout;
Its most commonly used as transformation of the entity's x and y coordinates, applied as translation, rotation, and scale

Docs I used:
-Vector2 : https://docs.godotengine.org/en/stable/classes/class_vector2.html, https://en.wikipedia.org/wiki/Two-dimensional_space
-Vector3 : https://docs.godotengine.org/en/stable/classes/class_vector3.html, https://en.wikipedia.org/wiki/Three-dimensional_space
-Vector4 : https://docs.godotengine.org/en/stable/classes/class_vector4.html, https://en.wikipedia.org/wiki/Four-dimensional_space, https://www.youtube.com/watch?v=0viYWQwIuZs
-Matrix  : https://www.geeksforgeeks.org/dsa/matrix/, https://docs.godotengine.org/en/latest/tutorials/math/matrices_and_transforms.html

*/

#ifndef DONUTMATH_H
#define DONUTMATH_H

#define DONUTMATH_VERSION "1.0"

#if defined(_WIN32)
	#ifdef DONUTMATH_EXPORTS
		#define DNMATH __declspec(dllexport)  
#elif defined(DONUTMATH_STATIC)
	#define DNMATH                         
#else
	#define DNMATH __declspec(dllimport)   
#endif
	#elif defined(BUILD_LIBTYPE_SHARED)
		#define DNMATH __attribute__((visibility("default")))
#else
	#define DNMATH
#endif

#include <limits>

// Basic Math defines

#ifndef PI
	constexpr float PI = 3.14159265359f;
#endif

#ifndef TUA
	constexpr float TUA = 6.28318530717f;
#endif

#ifndef E
	constexpr float E = 2.71828182845f;
#endif

#ifndef POSINF
	const float POSINF = std::numeric_limits<float>::infinity();
#endif

#ifndef NEGINF
	const float NEGINF = -std::numeric_limits<float>::infinity();
#endif

#ifndef EPSILON
	constexpr float EPSILON = 0.00000000001f;
#endif

#ifndef DEG2RAD
	constexpr float DEG2RAD = (PI / 180.0f);
#endif

#ifndef RAD2DEG
	constexpr float RAD2DEG = (180.0f / PI);
#endif

// Vector2
struct Vector2
{
	float x;
	float y;
};

DNMATH Vector2 Vector2Zero();                                                         // Sets vector x and y to 0
DNMATH Vector2 Vector2One();                                                          // Sets vector x and y to 1
DNMATH Vector2 Vector2Ten();                                                          // Sets vector x and y to 10
DNMATH Vector2 Vector2Add(Vector2 v1, Vector2 v2);                                    // Adds x and y stored values 
DNMATH Vector2 Vector2Subtract(Vector2 v1, Vector2 v2);                               // Subtracts x and y values
DNMATH Vector2 Vector2Multiply(Vector2 v1, Vector2 v2);                               // Multiply x and y values
DNMATH Vector2 Vector2Divide(Vector2 v1, Vector2 v2);                                 // Divide x and y values
DNMATH int Vector2Equal(Vector2 p, Vector2 q);                                        // Check if vector are almost equal
DNMATH float Vector2Length(Vector2 v);                                                // Gets length of the vector
DNMATH float Vector2Distance(Vector2 v1, Vector2 v2);                                 // Gets the distance between two vectors
DNMATH float Vector2Angle(Vector2 v1, Vector2 v2);                                    // Gets the angle between two vector
DNMATH Vector2 Vector2Scale(Vector2 v, float value);                                  // Scale vector by multiplying by value
DNMATH float Vector2Dot(Vector2 v1, Vector2 v2);                                      // Dot product of two vectors
DNMATH Vector2 Vector2Normalize(Vector2 v);                                           // Normalize vector to unit length
DNMATH Vector2 Vector2Rotate(Vector2 v, float angle);                                 // Rotate vector by an angle
DNMATH Vector2 Vector2MoveTowards(Vector2 v, Vector2 target, float maxDistance);      // Move vector towards a target position

// Vector3
struct Vector3
{
	float x;
	float y;
	float z;
};

DNMATH Vector3 Vector3Zero();                                                       // Sets vector x, y, and z to 0
DNMATH Vector3 Vector3One();                                                        // Sets vector x, y and z to 1
DNMATH Vector3 Vector3Ten();                                                        // Sets vector x, y, and z to 10
DNMATH Vector3 Vector3Add(Vector3 v1, Vector3 v2);                                  // Adds x, y and z stored values 
DNMATH Vector3 Vector3Subtract(Vector3 v1, Vector3 v2);                             // Subtracts x, y and z values
DNMATH Vector3 Vector3Multiply(Vector3 v1, Vector3 v2);                             // Multiply x, y and z values
DNMATH Vector3 Vector3Divide(Vector3 v1, Vector3 v2);                               // Divide x, y and z values
DNMATH int Vector3Equal(Vector3 p, Vector3 q);                                      // Check if vector are almost equal
DNMATH float Vector3Length(Vector3 v);                                              // Gets length of the vector
DNMATH float Vector3Distance(Vector3 v1, Vector3 v2);                               // Gets the distance between two vectors
DNMATH float Vector3Angle(Vector3 v1, Vector3 v2);                                  // Gets the angle between two vector
DNMATH Vector3 Vector3Scale(Vector3 v, float scale);                                // Scale vector by multiplying by scale value
DNMATH float Vector3Dot(Vector3 v1, Vector3 v2);                                    // Dot product of two vectors
DNMATH Vector3 Vector3Cross(Vector3 v1, Vector3 v2);                                // Cross product of two vectors
DNMATH Vector3 Vector3Normalize(Vector3 v);                                         // Normalize vector to unit length
DNMATH Vector3 Vector3RotateOnAxisAngle(Vector3 v, Vector3 axis, float angle);      // Rotates vector around an axis
DNMATH Vector3 Vector3MoveTowards(Vector3 v, Vector3 target, float maxDistance);    // Move vector towards a target position

// Vector4
struct Vector4
{
	float x;
	float y;
	float z;
	float w;
};

DNMATH Vector4 Vector4Zero();                                                       // Sets vector x, y, z, and w to 0
DNMATH Vector4 Vector4One();                                                        // Sets vector x, y, z, and w to 1
DNMATH Vector4 Vector4Ten();                                                        // Sets vector x, y, z, and w to 10
DNMATH Vector4 Vector4Add(Vector4 v1, Vector4 v2);                                  // Adds x, y, z, and w stored values 
DNMATH Vector4 Vector4Subtract(Vector4 v1, Vector4 v2);                             // Subtracts x, y, z, w values
DNMATH Vector4 Vector4Multiply(Vector4 v1, Vector4 v2);                             // Multiply x, y, z, w values
DNMATH Vector4 Vector4Divide(Vector4 v1, Vector4 v2);                               // Divide x, y, z, w values
DNMATH int Vector4Equal(Vector4 p, Vector4 q);                                      // Check if vector are almost equal
DNMATH float Vector4Length(Vector4 v);                                              // Gets length of the vector
DNMATH float Vector4Distance(Vector4 v1, Vector4 v2);                               // Gets the distance between two vectors
DNMATH float Vector4Dot(Vector4 v1, Vector4 v2);                                    // Dot product of two vectors
DNMATH Vector4 Vector4Normalize(Vector4 v);                                         // Normalize vector to unit length
DNMATH Vector4 Vector4Scale(Vector4 v, float scale);                                // Scale vector by multiplying by scale value
DNMATH Vector4 Vector4MoveTowards(Vector4 v, Vector4 target, float maxDistance);    // Move vector towards a target position

// 4D Rotation Plane Functions 

DNMATH Vector4 Rotate4DXY(Vector4 v, float angle);                                  // Rotate in XY plane (same as 3D Z-axis rotation)
DNMATH Vector4 Rotate4DXZ(Vector4 v, float angle);                                  // Rotate in XZ plane (same as 3D Y-axis rotation)
DNMATH Vector4 Rotate4DXW(Vector4 v, float angle);                                  // Rotate in XW plane (4D-specific: mixes X and W)
DNMATH Vector4 Rotate4DYZ(Vector4 v, float angle);                                  // Rotate in YZ plane (same as 3D X-axis rotation)
DNMATH Vector4 Rotate4DYW(Vector4 v, float angle);                                  // Rotate in YW plane (4D-specific: mixes Y and W)
DNMATH Vector4 Rotate4DZW(Vector4 v, float angle);                                  // Rotate in ZW plane (4D-specific: mixes Z and W)

// 3 by 3 Matrix
struct Matrix3
{
	float num0, num3, num6;
	float num1, num4, num7;
	float num2, num5, num8;
};

DNMATH Matrix3 Matrix3Add(Matrix3 m1, Matrix3 m2);            // Adds 2 Matrix3 values together
DNMATH Matrix3 Matrix3Subtract(Matrix3 m1, Matrix3 m2);       // Subtracts 2 Matrix3 values together
DNMATH Matrix3 Matrix3Multiply(Matrix3 m1, Matrix3 m2);       // Multiply 2 Matrix3 values together
DNMATH Matrix3 Matrix3Rotate(Vector3 axis, float angle);      // Rotates Matrix3 by axis and angle
DNMATH Matrix3 Matrix3RotateX(float angle);                   // Rotates Matrix3 by x
DNMATH Matrix3 Matrix3RotateY(float angle);                   // Rotates Matrix3 by y
DNMATH Matrix3 Matrix3RotateZ(float angle);                   // Rotates Matrix3 by z
DNMATH Matrix3 Matrix3RotateXYZ(Vector3 angle);               // Rotates Matrix3 by x, y, z
DNMATH Matrix3 Matrix3RotateZYX(Vector3 angle);               // Rotates Matrix3 by z, y, x

// 4 by 4 Matrix
struct Matrix4
{
	float num0, num4, num8, num12;
	float num1, num5, num9, num13;
	float num2, num6, num10, num14;
	float num3, num7, num11, num15;
};

DNMATH Matrix4 Matrix4Add(Matrix4 m1, Matrix4 m2);                                    // Adds 2 Matrix4 values together
DNMATH Matrix4 Matrix4Subtract(Matrix4 m1, Matrix4 m2);                               // Subtracts 2 Matrix4 values together
DNMATH Matrix4 Matrix4Multiply(Matrix4 m1, Matrix4 m2);                               // Multiply 2 Matrix4 values together
DNMATH Matrix4 Matrix4Identity();                                                     // Returns a 4x4 identity matrix
DNMATH Matrix4 Matrix4Rotate(Vector3 axis, float angle);                              // Rotates Matrix4 by axis (Vector3) and angle | NOTE: uses row-vector convention (v * M)
DNMATH Matrix4 Matrix4RotateX(float angle);                                           // Rotates Matrix4 by x
DNMATH Matrix4 Matrix4RotateY(float angle);                                           // Rotates Matrix4 by y
DNMATH Matrix4 Matrix4RotateZ(float angle);                                           // Rotates Matrix4 by z
DNMATH Matrix4 Matrix4RotateXYZ(Vector3 angle);                                       // Rotates Matrix4 by x, y, z
DNMATH Matrix4 Matrix4RotateZYX(Vector3 angle);                                       // Rotates Matrix4 by z, y, x
DNMATH Matrix4 Matrix4Translation(float tx, float ty, float tz);                      // Returns a translation matrix | row-vector convention: translation in last row
DNMATH Matrix4 Matrix4Scale(float sx, float sy, float sz);                            // Returns a scale matrix
DNMATH Matrix4 Matrix4Perspective(float fovY, float aspect, float nearZ, float farZ); // Returns a perspective projection matrix | row-vector convention (v * M); divide xyz by w after transform
DNMATH Matrix4 Matrix4LookAt(Vector3 eye, Vector3 center, Vector3 up);                // Takes three Vector3 positions to make a veiw matrix

// Quaternion Functions 

DNMATH Vector4 QuatIdentity();                                          // Returns (0,0,0,1) — no rotation
DNMATH Vector4 QuatFromAxisAngle(Vector3 axis, float angle);            // Build quaternion from axis + angle (radians)
DNMATH Vector4 QuatMultiply(Vector4 q1, Vector4 q2);                    // Combine two rotations  (q1 applied first, then q2)
DNMATH Vector4 QuatNormalize(Vector4 q);                                // Keep quaternion unit-length (call after many multiplies)
DNMATH Vector4 QuatConjugate(Vector4 q);                                // Conjugate (= inverse for unit quaternions)
DNMATH Vector4 QuatSlerp(Vector4 q1, Vector4 q2, float t);              // Smooth spherical interpolation; t in [0,1]
DNMATH Matrix4 QuatToMatrix4(Vector4 q);                                // Convert quaternion to a 4x4 rotation matrix
DNMATH Vector3 QuatRotateVector(Vector4 q, Vector3 v);                  // Rotate a 3D vector by a quaternion

// Moduler Math Functions 

DNMATH float Fade(float t);                                                                                // Calculates the value of the 5th-degree polynomial for a given input 
DNMATH float Grad(int hash, float x, float y);                                                             // Calculates a gradient from input used for perlin noise 
DNMATH float Lerp(float a, float b, float t);                                                              // Calculate linear interpolation between two floats
DNMATH float Perlin(float x, float y, const int* perm);                                                    // Perlin noise algorithm given input x, y and perm
DNMATH float Clamp(float value, float min, float max);                                                     // Clamp float value
DNMATH float Normalize(float value, float start, float end);                                               // Normalize input value within input range    
DNMATH float Wrap(float value, float min, float max);                                                      // Wrap input value from min to max
DNMATH float ReMap(float value, float inputStart, float inputEnd, float outputStart, float outputEnd);     // Remap input value within input range to output range
DNMATH float FloatEqual(float x, float y);                                                                 // Check if two floats are almost equal

#endif // !DONUTMATH_H