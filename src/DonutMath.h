/*
																		DonutMath 1.1 is a light weight terminal math API

Features:
-Basic math defines | PI, E, TUA, Epsilon, DEG2RAD, RAD2DEG
-Vector2 struct
-Vector3 struct
-Vector4 struct
-3 by 3 Matrix struct
-4 by 4 Matrix struct
-Math helpers

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

#pragma once

#ifndef DONUTMATH_H
#define DONUTMATH_H

#define DONUTMATH_VERSION "1.1"

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

// DonutAPI_C.h includes this header, so everything below has to parse as C too.
#ifdef __cplusplus
	#include <limits>
	#define DNMATH_CONST constexpr float
extern "C" {
#else
	#include <math.h>
	#define DNMATH_CONST static const float
#endif

// Basic Math defines

#ifndef PI
	DNMATH_CONST PI = 3.14159265359f;
#endif

#ifndef TUA
	DNMATH_CONST TUA = 6.28318530717f;
#endif

#ifndef E
	DNMATH_CONST E = 2.71828182845f;
#endif

#ifdef __cplusplus
	#ifndef POSINF
		const float POSINF = std::numeric_limits<float>::infinity();
#endif

#ifndef NEGINF
	const float NEGINF = -std::numeric_limits<float>::infinity();
#endif
#else
#ifndef POSINF
	static const float POSINF = (float)INFINITY;
#endif

#ifndef NEGINF
	static const float NEGINF = -(float)INFINITY;
#endif
#endif

#ifndef EPSILON
	DNMATH_CONST EPSILON = 0.000001f;
#endif

#ifndef DEG2RAD
	DNMATH_CONST DEG2RAD = (PI / 180.0f);
#endif

#ifndef RAD2DEG
	DNMATH_CONST RAD2DEG = (180.0f / PI);
#endif



// Vector2



typedef struct Vector2
{
	float x;
	float y;
} Vector2;

// Sets vector x and y to 0
DNMATH Vector2 Vector2Zero();

// Sets vector x and y to 1
DNMATH Vector2 Vector2One();

// Sets vector x and y to 10
DNMATH Vector2 Vector2Ten();

// Adds x and y stored values 
DNMATH Vector2 Vector2Add(Vector2 v1, Vector2 v2);

// Subtracts x and y values
DNMATH Vector2 Vector2Subtract(Vector2 v1, Vector2 v2);

// Multiply x and y values
DNMATH Vector2 Vector2Multiply(Vector2 v1, Vector2 v2);

// Divide x and y values
DNMATH Vector2 Vector2Divide(Vector2 v1, Vector2 v2);

// Check if vector are almost equal
DNMATH int Vector2Equal(Vector2 p, Vector2 q);

// Gets length of the vector
DNMATH float Vector2Length(Vector2 v);

// Gets the distance between two vectors
DNMATH float Vector2Distance(Vector2 v1, Vector2 v2);

// Gets the angle between two vector
DNMATH float Vector2Angle(Vector2 v1, Vector2 v2);

// Scale vector by multiplying by value
DNMATH Vector2 Vector2Scale(Vector2 v, float value);

// Dot product of two vectors
DNMATH float Vector2Dot(Vector2 v1, Vector2 v2);

// Normalize vector to unit length
DNMATH Vector2 Vector2Normalize(Vector2 v);

// Rotate vector by an angle
DNMATH Vector2 Vector2Rotate(Vector2 v, float angle);

// Move vector towards a target position
DNMATH Vector2 Vector2MoveTowards(Vector2 v, Vector2 target, float maxDistance);



// Vector3



typedef struct Vector3
{
	float x;
	float y;
	float z;
} Vector3;

// Sets vector x, y, and z to 0
DNMATH Vector3 Vector3Zero();

// Sets vector x, y and z to 1
DNMATH Vector3 Vector3One();

// Sets vector x, y, and z to 10
DNMATH Vector3 Vector3Ten();

// Adds x, y and z stored values
DNMATH Vector3 Vector3Add(Vector3 v1, Vector3 v2);

// Subtracts x, y and z values
DNMATH Vector3 Vector3Subtract(Vector3 v1, Vector3 v2);

// Multiply x, y and z values
DNMATH Vector3 Vector3Multiply(Vector3 v1, Vector3 v2);

// Divide x, y and z values
DNMATH Vector3 Vector3Divide(Vector3 v1, Vector3 v2);

// Check if vector are almost equal
DNMATH int Vector3Equal(Vector3 p, Vector3 q);

// Gets length of the vector
DNMATH float Vector3Length(Vector3 v);

// Gets the distance between two vectors
DNMATH float Vector3Distance(Vector3 v1, Vector3 v2);

// Gets the angle between two vector
DNMATH float Vector3Angle(Vector3 v1, Vector3 v2);

// Scale vector by multiplying by scale value
DNMATH Vector3 Vector3Scale(Vector3 v, float scale);

// Dot product of two vectors
DNMATH float Vector3Dot(Vector3 v1, Vector3 v2);

// Cross product of two vectors
DNMATH Vector3 Vector3Cross(Vector3 v1, Vector3 v2);

// Normalize vector to unit length
DNMATH Vector3 Vector3Normalize(Vector3 v);

// Rotates vector around an axis
DNMATH Vector3 Vector3RotateOnAxisAngle(Vector3 v, Vector3 axis, float angle);

// Move vector towards a target position
DNMATH Vector3 Vector3MoveTowards(Vector3 v, Vector3 target, float maxDistance);



// Vector4



typedef struct Vector4
{
	float x;
	float y;
	float z;
	float w;
} Vector4;



// Sets vector x, y, z, and w to 0
DNMATH Vector4 Vector4Zero();

// Sets vector x, y, z, and w to 1
DNMATH Vector4 Vector4One();

// Sets vector x, y, z, and w to 10
DNMATH Vector4 Vector4Ten();

// Adds x, y, z, and w stored values
DNMATH Vector4 Vector4Add(Vector4 v1, Vector4 v2);

// Subtracts x, y, z, w values
DNMATH Vector4 Vector4Subtract(Vector4 v1, Vector4 v2);

// Multiply x, y, z, w values
DNMATH Vector4 Vector4Multiply(Vector4 v1, Vector4 v2);

// Divide x, y, z, w values
DNMATH Vector4 Vector4Divide(Vector4 v1, Vector4 v2);

// Check if vector are almost equal
DNMATH int Vector4Equal(Vector4 p, Vector4 q);

// Gets length of the vector
DNMATH float Vector4Length(Vector4 v);

// Gets the distance between two vectors
DNMATH float Vector4Distance(Vector4 v1, Vector4 v2);

// Dot product of two vectors
DNMATH float Vector4Dot(Vector4 v1, Vector4 v2);

// Normalize vector to unit length
DNMATH Vector4 Vector4Normalize(Vector4 v);

// Scale vector by multiplying by scale value
DNMATH Vector4 Vector4Scale(Vector4 v, float scale);

// Move vector towards a target position
DNMATH Vector4 Vector4MoveTowards(Vector4 v, Vector4 target, float maxDistance);



// 4D Rotation Plane Functions 



// Rotate in XY plane (same as 3D Z-axis rotation)
DNMATH Vector4 Rotate4DXY(Vector4 v, float angle);

// Rotate in XZ plane (same as 3D Y-axis rotation)
DNMATH Vector4 Rotate4DXZ(Vector4 v, float angle);

// Rotate in XW plane (4D-specific: mixes X and W)
DNMATH Vector4 Rotate4DXW(Vector4 v, float angle);

// Rotate in YZ plane (same as 3D X-axis rotation)
DNMATH Vector4 Rotate4DYZ(Vector4 v, float angle);

// Rotate in YW plane (4D-specific: mixes Y and W)
DNMATH Vector4 Rotate4DYW(Vector4 v, float angle);

// Rotate in ZW plane (4D-specific: mixes Z and W)
DNMATH Vector4 Rotate4DZW(Vector4 v, float angle);



// 3 by 3 Matrix
typedef struct Matrix3
{
	float num0, num3, num6;
	float num1, num4, num7;
	float num2, num5, num8;
} Matrix3;



// Adds 2 Matrix3 values together
DNMATH Matrix3 Matrix3Add(Matrix3 m1, Matrix3 m2);

// Subtracts 2 Matrix3 values together
DNMATH Matrix3 Matrix3Subtract(Matrix3 m1, Matrix3 m2);

// Multiply 2 Matrix3 values together
DNMATH Matrix3 Matrix3Multiply(Matrix3 m1, Matrix3 m2);

// Rotates Matrix3 by axis and angle
DNMATH Matrix3 Matrix3Rotate(Vector3 axis, float angle);

// Rotates Matrix3 by x angle
DNMATH Matrix3 Matrix3RotateX(float angle);

// Rotates Matrix3 by y angle
DNMATH Matrix3 Matrix3RotateY(float angle);

// Rotates Matrix3 by z angle
DNMATH Matrix3 Matrix3RotateZ(float angle);

// Rotates Matrix3 by x, y, z 
DNMATH Matrix3 Matrix3RotateXYZ(Vector3 angle);

// Rotates Matrix3 by z, y, x
DNMATH Matrix3 Matrix3RotateZYX(Vector3 angle);



// 4 by 4 Matrix
typedef struct Matrix4
{
	float num0, num4, num8, num12;
	float num1, num5, num9, num13;
	float num2, num6, num10, num14;
	float num3, num7, num11, num15;
} Matrix4;



// Adds 2 Matrix4 values together
DNMATH Matrix4 Matrix4Add(Matrix4 m1, Matrix4 m2);

// Subtracts 2 Matrix4 values together
DNMATH Matrix4 Matrix4Subtract(Matrix4 m1, Matrix4 m2);

// Multiply 2 Matrix4 values together
DNMATH Matrix4 Matrix4Multiply(Matrix4 m1, Matrix4 m2);

// Returns a 4x4 identity matrix
DNMATH Matrix4 Matrix4Identity();

// Rotates Matrix4 by axis (Vector3) and angle | NOTE: uses row-vector convention (v * M)
DNMATH Matrix4 Matrix4Rotate(Vector3 axis, float angle);

// Rotates Matrix4 by x angle
DNMATH Matrix4 Matrix4RotateX(float angle);

// Rotates Matrix4 by y angle
DNMATH Matrix4 Matrix4RotateY(float angle);

// Rotates Matrix4 by z angle
DNMATH Matrix4 Matrix4RotateZ(float angle);

// Rotates Matrix4 by x, y, z
DNMATH Matrix4 Matrix4RotateXYZ(Vector3 angle);

// Rotates Matrix4 by z, y, x
DNMATH Matrix4 Matrix4RotateZYX(Vector3 angle);

// Returns a translation matrix | row-vector convention: translation in last row
DNMATH Matrix4 Matrix4Translation(float tx, float ty, float tz);

// Returns a scale matrix
DNMATH Matrix4 Matrix4Scale(float sx, float sy, float sz);

// Returns a perspective projection matrix | row-vector convention (v * M); divide xyz by w after transform
DNMATH Matrix4 Matrix4Perspective(float fovY, float aspect, float nearZ, float farZ);

// Takes three Vector3 positions to make a veiw matrix
DNMATH Matrix4 Matrix4LookAt(Vector3 eye, Vector3 center, Vector3 up);



// Quaternion Functions 



// Returns (0,0,0,1) — no rotation
DNMATH Vector4 QuatIdentity();

// Build quaternion from axis + angle (radians)
DNMATH Vector4 QuatFromAxisAngle(Vector3 axis, float angle);

// Combine two rotations  (q1 applied first, then q2)
DNMATH Vector4 QuatMultiply(Vector4 q1, Vector4 q2);

// Keep quaternion unit-length (call after many multiplies)
DNMATH Vector4 QuatNormalize(Vector4 q);

// Conjugate (= inverse for unit quaternions)
DNMATH Vector4 QuatConjugate(Vector4 q);

// Smooth spherical interpolation; t in [0,1]
DNMATH Vector4 QuatSlerp(Vector4 q1, Vector4 q2, float t);

// Convert quaternion to a 4x4 rotation matrix
DNMATH Matrix4 QuatToMatrix4(Vector4 q);

// Rotate a 3D vector by a quaternion
DNMATH Vector3 QuatRotateVector(Vector4 q, Vector3 v);



// Moduler Math Functions 



// Calculates the value of the 5th-degree polynomial for a given input
DNMATH float Fade(float t);

// Calculates a gradient from input used for perlin noise 
DNMATH float Grad(int hash, float x, float y);

// Calculate linear interpolation between two floats
DNMATH float Lerp(float a, float b, float t);

// Perlin noise algorithm given input x, y and perm
DNMATH float Perlin(float x, float y, const int* perm);

// Clamp float value
DNMATH float Clamp(float value, float min, float max);

// Normalize input value within input range 
DNMATH float Normalize(float value, float start, float end);

// Wrap input value from min to max
DNMATH float Wrap(float value, float min, float max);

// Remap input value within input range to output range
DNMATH float ReMap(float value, float inputStart, float inputEnd, float outputStart, float outputEnd);

// Check if two floats are almost equal
DNMATH float FloatEqual(float x, float y);

DNMATH float EaseInOutQuad(float t);

DNMATH float EaseOutBounce(float t);

DNMATH float EaseOutElastic(float t);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // !DONUTMATH_H
