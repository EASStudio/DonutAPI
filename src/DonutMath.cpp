#include "DonutMath.h"

#include <cmath>
#include <algorithm>

// Vector2 Functions

DNMATH Vector2 Vector2Zero()
{
	Vector2 result = { 0.0f, 0.0f };

	return result;
}

DNMATH Vector2 Vector2One()
{
	Vector2 result = { 1.0f, 1.0f };

	return result;;
}

DNMATH Vector2 Vector2Ten()
{
	Vector2 result = { 10.0f, 10.0f };

	return result;
}

DNMATH Vector2 Vector2Add(Vector2 v1, Vector2 v2)
{
	Vector2 result{ v1.x + v2.x, v1.y + v2.y };

	return result;
}

DNMATH Vector2 Vector2Subtract(Vector2 v1, Vector2 v2)
{
	Vector2 result = { v1.x - v2.x, v1.y - v2.y };

	return result;
}

DNMATH Vector2 Vector2Multiply(Vector2 v1, Vector2 v2)
{
	Vector2 result = { v1.x * v2.x, v1.y * v2.y };

	return result;
}

DNMATH Vector2 Vector2Divide(Vector2 v1, Vector2 v2)
{
	Vector2 result = { v1.x / v2.x, v1.y / v2.y };

	return result;
}

DNMATH int Vector2Equal(Vector2 p, Vector2 q)
{
	auto nearlyEqual = [](float a, float b)
		{
			return std::abs(a - b) <= EPSILON * std::max(1.0f, std::max(std::abs(a), std::abs(b)));
		};

	return nearlyEqual(p.x, q.x) && nearlyEqual(p.y, q.y);
}

DNMATH float Vector2Length(Vector2 v)
{
	float result = sqrtf((v.x * v.x) + (v.y * v.y));

	return result;
}

DNMATH float Vector2Distance(Vector2 v1, Vector2 v2)
{
	float result = sqrtf((v1.x - v2.x) * (v1.x - v2.x) + (v1.y - v2.y) * (v1.y - v2.y));

	return result;
}

DNMATH float Vector2Angle(Vector2 v1, Vector2 v2)
{
	float result = 0.0f;

	float dot = (v1.x * v2.x) + (v1.y * v2.y);
	float det = (v1.x * v2.y) - (v1.y * v2.x);

	result = atan2f(det, dot);

	return result;
}

DNMATH Vector2 Vector2Scale(Vector2 v, float scale)
{
	Vector2 result = { v.x * scale, v.y * scale };

	return result;
}

DNMATH float Vector2Dot(Vector2 v1, Vector2 v2)
{
	return (v1.x * v2.x) + (v1.y * v2.y);
}

DNMATH Vector2 Vector2Normalize(Vector2 v)
{
	float len = sqrtf((v.x * v.x) + (v.y * v.y));
	if (len == 0.0f) return { 0.0f, 0.0f };

	float inv = 1.0f / len;
	return { v.x * inv, v.y * inv };
}

DNMATH Vector2 Vector2Rotate(Vector2 v, float angle)
{
	Vector2 result = { 0 };

	float cosres = cosf(angle);
	float sinres = sinf(angle);

	result.x = v.x * cosres - v.y * sinres;
	result.y = v.x * sinres + v.y * cosres;

	return result;
}

DNMATH Vector2 Vector2MoveTowards(Vector2 v, Vector2 target, float maxDistance)
{
	Vector2 result = { 0 };

	float dx = target.x - v.x;
	float dy = target.y - v.y;
	float value = (dx * dx) + (dy * dy);

	if ((value == 0) || ((maxDistance >= 0) && (value <= maxDistance * maxDistance))) return target;

	float dist = sqrtf(value);

	result.x = v.x + dx / dist * maxDistance;
	result.y = v.y + dy / dist * maxDistance;

	return result;
}


// Vector3 Functions


DNMATH Vector3 Vector3Zero()
{
	Vector3 result = { 0.0f, 0.0f, 0.0f };

	return result;
}

DNMATH Vector3 Vector3One()
{
	Vector3 result = { 1.0f, 1.0f, 1.0f };

	return result;
}

DNMATH Vector3 Vector3Ten()
{
	Vector3 result = { 10.0f, 10.0f, 10.0f };

	return result;
}

DNMATH Vector3 Vector3Add(Vector3 v1, Vector3 v2)
{
	Vector3 result = { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };

	return result;
}

DNMATH Vector3 Vector3Subtract(Vector3 v1, Vector3 v2)
{
	Vector3 result = { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };

	return result;
}

DNMATH Vector3 Vector3Multiply(Vector3 v1, Vector3 v2)
{
	Vector3 result = { v1.x * v2.x, v1.y * v2.y, v1.z * v2.z };

	return result;
}

DNMATH Vector3 Vector3Divide(Vector3 v1, Vector3 v2)
{
	Vector3 result = { v1.x / v2.x, v1.y / v2.y, v1.z / v2.z };

	return result;
}

DNMATH int Vector3Equal(Vector3 p, Vector3 q)
{
	auto nearlyEqual = [](float a, float b)
		{
			return std::abs(a - b) <= EPSILON * std::max(1.0f, std::max(std::abs(a), std::abs(b)));
		};

	return nearlyEqual(p.x, q.x) && nearlyEqual(p.y, q.y) && nearlyEqual(p.z, q.z);
}

DNMATH float Vector3Length(Vector3 v)
{
	float result = sqrtf((v.x * v.x) + (v.y * v.y) + (v.z * v.z));

	return result;
}

DNMATH float Vector3Distance(Vector3 v1, Vector3 v2)
{
	float result = 0.0f;

	float dx = v2.x - v1.x;
	float dy = v2.y - v1.y;
	float dz = v2.z - v1.z;
	result = sqrtf((dx * dx) + (dy * dy) + (dz * dz));

	return result;
}

DNMATH float Vector3Angle(Vector3 v1, Vector3 v2)
{
	float result = 0.0f;

	Vector3 cross = { (v1.y * v2.z) - (v1.z * v2.y), (v1.z * v2.x) - (v1.x * v2.z), (v1.x * v2.y) - (v1.y * v2.x) };
	float len = sqrtf((cross.x * cross.x) + (cross.y * cross.y) + (cross.z * cross.z));
	float dot = ((v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z));
	result = atan2f(len, dot);

	return result;
}

DNMATH Vector3 Vector3Scale(Vector3 v, float scale)
{
	Vector3 result = { v.x * scale, v.y * scale, v.z * scale };

	return result;
}

DNMATH float Vector3Dot(Vector3 v1, Vector3 v2)
{
	return (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z);
}

DNMATH Vector3 Vector3Cross(Vector3 v1, Vector3 v2)
{
	Vector3 result =
	{
		(v1.y * v2.z) - (v1.z * v2.y),
		(v1.z * v2.x) - (v1.x * v2.z),
		(v1.x * v2.y) - (v1.y * v2.x)
	};

	return result;
}

DNMATH Vector3 Vector3Normalize(Vector3 v)
{
	float len = sqrtf((v.x * v.x) + (v.y * v.y) + (v.z * v.z));
	if (len == 0.0f) return { 0.0f, 0.0f, 0.0f };

	float inv = 1.0f / len;
	return { v.x * inv, v.y * inv, v.z * inv };
}

DNMATH Vector3 Vector3RotateOnAxisAngle(Vector3 v, Vector3 axis, float angle)
{
	Vector3 result = v;

	float length = sqrtf((axis.x * axis.x) + (axis.y * axis.y) + (axis.z * axis.z));
	if (length == 0.0f) length = 1.0f;
	float ilength = 1.0f / length;
	axis.x *= ilength;
	axis.y *= ilength;
	axis.z *= ilength;

	angle /= 2.0f;
	float a = sinf(angle);
	float b = axis.x * a;
	float c = axis.y * a;
	float d = axis.z * a;
	a = cosf(angle);
	Vector3 w = { b, c, d };

	Vector3 wv = { (w.y * v.z) - (w.z * v.y), (w.z * v.x) - (w.x * v.z), (w.x * v.y) - (w.y * v.x) };

	Vector3 wwv = { (w.y * wv.z) - (w.z * wv.y), (w.z * wv.x) - (w.x * wv.z), (w.x * wv.y) - (w.y * wv.x) };

	a *= 2;
	wv.x *= a;
	wv.y *= a;
	wv.z *= a;

	wwv.x *= 2;
	wwv.y *= 2;
	wwv.z *= 2;

	result.x += wv.x;
	result.y += wv.y;
	result.z += wv.z;

	result.x += wwv.x;
	result.y += wwv.y;
	result.z += wwv.z;

	return result;
}

DNMATH Vector3 Vector3MoveTowards(Vector3 v, Vector3 target, float maxDistance)
{
	Vector3 result = { 0 };

	float dx = target.x - v.x;
	float dy = target.y - v.y;
	float dz = target.z - v.z;
	float value = (dx * dx) + (dy * dy) + (dz * dz);

	if ((value == 0) || ((maxDistance >= 0) && (value <= maxDistance * maxDistance))) return target;

	float dist = sqrtf(value);

	result.x = v.x + dx / dist * maxDistance;
	result.y = v.y + dy / dist * maxDistance;
	result.z = v.z + dz / dist * maxDistance;

	return result;
}

// Vector4 Functions


DNMATH Vector4 Vector4Zero()
{
	Vector4 result = { 0.0f, 0.0f, 0.0f, 0.0f };

	return result;
}

DNMATH Vector4 Vector4One()
{
	Vector4 result = { 1.0f, 1.0f, 1.0f, 1.0f };

	return result;
}

DNMATH Vector4 Vector4Ten()
{
	Vector4 result = { 10.0f, 10.0f, 10.0f, 10.0f };

	return result;
}

DNMATH Vector4 Vector4Add(Vector4 v1, Vector4 v2)
{
	Vector4 result = { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w };

	return result;
}

DNMATH Vector4 Vector4Subtract(Vector4 v1, Vector4 v2)
{
	Vector4 result = { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w };

	return result;
}

DNMATH Vector4 Vector4Multiply(Vector4 v1, Vector4 v2)
{
	Vector4 result = { v1.x * v2.x, v1.y * v2.y, v1.z * v2.z, v1.w * v2.w };

	return result;
}

DNMATH Vector4 Vector4Divide(Vector4 v1, Vector4 v2)
{
	Vector4 result = { v1.x / v2.x, v1.y / v2.y, v1.z / v2.z, v1.w / v2.w };

	return result;
}

DNMATH int Vector4Equal(Vector4 p, Vector4 q)
{
	auto nearlyEqual = [](float a, float b)
		{
			return std::abs(a - b) <= EPSILON * std::max(1.0f, std::max(std::abs(a), std::abs(b)));
		};

	return nearlyEqual(p.x, q.x) && nearlyEqual(p.y, q.y) && nearlyEqual(p.z, q.z) && nearlyEqual(p.w, q.w);
}

DNMATH float Vector4Length(Vector4 v)
{
	float result = sqrtf((v.x * v.x) + (v.y * v.y) + (v.z * v.z) + (v.w * v.w));

	return result;
}

DNMATH float Vector4Distance(Vector4 v1, Vector4 v2)
{
	float result = 0.0f;

	float dx = v2.x - v1.x;
	float dy = v2.y - v1.y;
	float dz = v2.z - v1.z;
	float dw = v2.w - v1.w;
	result = sqrtf((dx * dx) + (dy * dy) + (dz * dz) + (dw * dw));

	return result;
}

DNMATH float Vector4Dot(Vector4 v1, Vector4 v2)
{
	return (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z) + (v1.w * v2.w);
}

DNMATH Vector4 Vector4Normalize(Vector4 v)
{
	float len = sqrtf((v.x * v.x) + (v.y * v.y) + (v.z * v.z) + (v.w * v.w));
	if (len == 0.0f) return { 0.0f, 0.0f, 0.0f, 0.0f };

	float inv = 1.0f / len;
	return { v.x * inv, v.y * inv, v.z * inv, v.w * inv };
}

DNMATH Vector4 Vector4Scale(Vector4 v, float scale)
{
	Vector4 result = { v.x * scale, v.y * scale, v.z * scale, v.w * scale };

	return result;
}

DNMATH Vector4 Vector4MoveTowards(Vector4 v, Vector4 target, float maxDistance)
{
	Vector4 result = { 0 };

	float dx = target.x - v.x;
	float dy = target.y - v.y;
	float dz = target.z - v.z;
	float dw = target.w - v.w;
	float value = (dx * dx) + (dy * dy) + (dz * dz) + (dw * dw);

	if ((value == 0) || ((maxDistance >= 0) && (value <= maxDistance * maxDistance))) return target;

	float dist = sqrtf(value);

	result.x = v.x + dx / dist * maxDistance;
	result.y = v.y + dy / dist * maxDistance;
	result.z = v.z + dz / dist * maxDistance;
	result.w = v.w + dw / dist * maxDistance;

	return result;
}


// 4D Rotation Plane Functions 


DNMATH Vector4 Rotate4DXY(Vector4 v, float angle)
{
	float c = cosf(angle), s = sinf(angle);
	return { v.x * c - v.y * s,  v.x * s + v.y * c,  v.z,  v.w };
}

DNMATH Vector4 Rotate4DXZ(Vector4 v, float angle)
{
	float c = cosf(angle), s = sinf(angle);
	return { v.x * c - v.z * s,  v.y,  v.x * s + v.z * c,  v.w };
}

DNMATH Vector4 Rotate4DXW(Vector4 v, float angle)
{
	float c = cosf(angle), s = sinf(angle);
	return { v.x * c - v.w * s,  v.y,  v.z,  v.x * s + v.w * c };
}

DNMATH Vector4 Rotate4DYZ(Vector4 v, float angle)
{
	float c = cosf(angle), s = sinf(angle);
	return { v.x,  v.y * c - v.z * s,  v.y * s + v.z * c,  v.w };
}

DNMATH Vector4 Rotate4DYW(Vector4 v, float angle)
{
	float c = cosf(angle), s = sinf(angle);
	return { v.x,  v.y * c - v.w * s,  v.z,  v.y * s + v.w * c };
}

DNMATH Vector4 Rotate4DZW(Vector4 v, float angle)
{
	float c = cosf(angle), s = sinf(angle);
	return { v.x,  v.y,  v.z * c - v.w * s,  v.z * s + v.w * c };
}


// Matrix3 Functions


DNMATH Matrix3 Matrix3Add(Matrix3 m1, Matrix3 m2)
{
	Matrix3 result = { 0 };

	result.num0 = m1.num0 + m2.num0;
	result.num1 = m1.num1 + m2.num1;
	result.num2 = m1.num2 + m2.num2;
	result.num3 = m1.num3 + m2.num3;
	result.num4 = m1.num4 + m2.num4;
	result.num5 = m1.num5 + m2.num5;
	result.num6 = m1.num6 + m2.num6;
	result.num7 = m1.num7 + m2.num7;
	result.num8 = m1.num8 + m2.num8;

	return result;
}

DNMATH Matrix3 Matrix3Subtract(Matrix3 m1, Matrix3 m2)
{
	Matrix3 result = { 0 };

	result.num0 = m1.num0 - m2.num0;
	result.num1 = m1.num1 - m2.num1;
	result.num2 = m1.num2 - m2.num2;
	result.num3 = m1.num3 - m2.num3;
	result.num4 = m1.num4 - m2.num4;
	result.num5 = m1.num5 - m2.num5;
	result.num6 = m1.num6 - m2.num6;
	result.num7 = m1.num7 - m2.num7;
	result.num8 = m1.num8 - m2.num8;

	return result;
}

DNMATH Matrix3 Matrix3Multiply(Matrix3 m1, Matrix3 m2)
{
	Matrix3 result = { 0 };

	result.num0 = (m1.num0 * m2.num0) + (m1.num1 * m2.num3) + (m1.num2 * m2.num6);
	result.num1 = (m1.num0 * m2.num1) + (m1.num1 * m2.num4) + (m1.num2 * m2.num7);
	result.num2 = (m1.num0 * m2.num2) + (m1.num1 * m2.num5) + (m1.num2 * m2.num8);
	result.num3 = (m1.num3 * m2.num0) + (m1.num4 * m2.num3) + (m1.num5 * m2.num6);
	result.num4 = (m1.num3 * m2.num1) + (m1.num4 * m2.num4) + (m1.num5 * m2.num7);
	result.num5 = (m1.num3 * m2.num2) + (m1.num4 * m2.num5) + (m1.num5 * m2.num8);
	result.num6 = (m1.num6 * m2.num0) + (m1.num7 * m2.num3) + (m1.num8 * m2.num6);
	result.num7 = (m1.num6 * m2.num1) + (m1.num7 * m2.num4) + (m1.num8 * m2.num7);
	result.num8 = (m1.num6 * m2.num2) + (m1.num7 * m2.num5) + (m1.num8 * m2.num8);

	return result;
}

DNMATH Matrix3 Matrix3Rotate(Vector3 axis, float angle)
{
	Matrix3 result = { 0 };

	float x = axis.x, y = axis.y, z = axis.z;

	float lengthSquared = (x * x) + (y * y) + (z * z);

	if ((lengthSquared != 1.0f) && (lengthSquared != 0.0f))
	{
		float ilength = 1.0f / sqrtf(lengthSquared);
		x *= ilength;
		y *= ilength;
		z *= ilength;
	}

	float sinres = sinf(angle);
	float cosres = cosf(angle);
	float t = 1.0f - cosres;

	result.num0 = x * x * t + cosres;
	result.num1 = x * y * t - z * sinres;
	result.num2 = x * z * t + y * sinres;

	result.num3 = x * y * t + z * sinres;
	result.num4 = y * y * t + cosres;
	result.num5 = y * z * t - x * sinres;

	result.num6 = x * z * t - y * sinres;
	result.num7 = y * z * t + x * sinres;
	result.num8 = z * z * t + cosres;

	return result;
}

DNMATH Matrix3 Matrix3RotateX(float angle)
{
	Matrix3 result = { 0 };

	float sinres = sinf(angle);
	float cosres = cosf(angle);

	result.num0 = 1.0f;
	result.num1 = 0.0f;
	result.num2 = 0.0f;

	result.num3 = 0.0f;
	result.num4 = cosres;
	result.num5 = -sinres;

	result.num6 = 0.0f;
	result.num7 = sinres;
	result.num8 = cosres;

	return result;
}

DNMATH Matrix3 Matrix3RotateY(float angle)
{
	Matrix3 result = { 0 };

	float sinres = sinf(angle);
	float cosres = cosf(angle);

	result.num0 = cosres;
	result.num1 = 0.0f;
	result.num2 = sinres;

	result.num3 = 0.0f;
	result.num4 = 1.0f;
	result.num5 = 0.0f;

	result.num6 = -sinres;
	result.num7 = 0.0f;
	result.num8 = cosres;

	return result;
}

DNMATH Matrix3 Matrix3RotateZ(float angle)
{
	Matrix3 result = { 0 };

	float sinres = sinf(angle);
	float cosres = cosf(angle);

	result.num0 = cosres;
	result.num1 = -sinres;
	result.num2 = 0.0f;

	result.num3 = sinres;
	result.num4 = cosres;
	result.num5 = 0.0f;

	result.num6 = 0.0f;
	result.num7 = 0.0f;
	result.num8 = 1.0f;

	return result;
}

DNMATH Matrix3 Matrix3RotateXYZ(Vector3 angle)
{
	Matrix3 rx = Matrix3RotateX(angle.x);
	Matrix3 ry = Matrix3RotateY(angle.y);
	Matrix3 rz = Matrix3RotateZ(angle.z);

	return Matrix3Multiply(Matrix3Multiply(rz, ry), rx);
}

DNMATH Matrix3 Matrix3RotateZYX(Vector3 angle)
{
	Matrix3 rx = Matrix3RotateX(angle.x);
	Matrix3 ry = Matrix3RotateY(angle.y);
	Matrix3 rz = Matrix3RotateZ(angle.z);

	return Matrix3Multiply(Matrix3Multiply(rx, ry), rz);
}


// Matrix4 Functions


DNMATH Matrix4 Matrix4Add(Matrix4 m1, Matrix4 m2)
{
	Matrix4 result = { 0 };

	result.num0 = m1.num0 + m2.num0;
	result.num1 = m1.num1 + m2.num1;
	result.num2 = m1.num2 + m2.num2;
	result.num3 = m1.num3 + m2.num3;
	result.num4 = m1.num4 + m2.num4;
	result.num5 = m1.num5 + m2.num5;
	result.num6 = m1.num6 + m2.num6;
	result.num7 = m1.num7 + m2.num7;
	result.num8 = m1.num8 + m2.num8;
	result.num9 = m1.num9 + m2.num9;
	result.num10 = m1.num10 + m2.num10;
	result.num11 = m1.num11 + m2.num11;
	result.num12 = m1.num12 + m2.num12;
	result.num13 = m1.num13 + m2.num13;
	result.num14 = m1.num14 + m2.num14;
	result.num15 = m1.num15 + m2.num15;

	return result;
}

DNMATH Matrix4 Matrix4Subtract(Matrix4 m1, Matrix4 m2)
{
	Matrix4 result = { 0 };

	result.num0 = m1.num0 - m2.num0;
	result.num1 = m1.num1 - m2.num1;
	result.num2 = m1.num2 - m2.num2;
	result.num3 = m1.num3 - m2.num3;
	result.num4 = m1.num4 - m2.num4;
	result.num5 = m1.num5 - m2.num5;
	result.num6 = m1.num6 - m2.num6;
	result.num7 = m1.num7 - m2.num7;
	result.num8 = m1.num8 - m2.num8;
	result.num9 = m1.num9 - m2.num9;
	result.num10 = m1.num10 - m2.num10;
	result.num11 = m1.num11 - m2.num11;
	result.num12 = m1.num12 - m2.num12;
	result.num13 = m1.num13 - m2.num13;
	result.num14 = m1.num14 - m2.num14;
	result.num15 = m1.num15 - m2.num15;

	return result;
}

DNMATH Matrix4 Matrix4Multiply(Matrix4 m1, Matrix4 m2)
{
	Matrix4 result = { 0 };

	result.num0 = m1.num0 * m2.num0 + m1.num1 * m2.num4 + m1.num2 * m2.num8 + m1.num3 * m2.num12;
	result.num1 = m1.num0 * m2.num1 + m1.num1 * m2.num5 + m1.num2 * m2.num9 + m1.num3 * m2.num13;
	result.num2 = m1.num0 * m2.num2 + m1.num1 * m2.num6 + m1.num2 * m2.num10 + m1.num3 * m2.num14;
	result.num3 = m1.num0 * m2.num3 + m1.num1 * m2.num7 + m1.num2 * m2.num11 + m1.num3 * m2.num15;
	result.num4 = m1.num4 * m2.num0 + m1.num5 * m2.num4 + m1.num6 * m2.num8 + m1.num7 * m2.num12;
	result.num5 = m1.num4 * m2.num1 + m1.num5 * m2.num5 + m1.num6 * m2.num9 + m1.num7 * m2.num13;
	result.num6 = m1.num4 * m2.num2 + m1.num5 * m2.num6 + m1.num6 * m2.num10 + m1.num7 * m2.num14;
	result.num7 = m1.num4 * m2.num3 + m1.num5 * m2.num7 + m1.num6 * m2.num11 + m1.num7 * m2.num15;
	result.num8 = m1.num8 * m2.num0 + m1.num9 * m2.num4 + m1.num10 * m2.num8 + m1.num11 * m2.num12;
	result.num9 = m1.num8 * m2.num1 + m1.num9 * m2.num5 + m1.num10 * m2.num9 + m1.num11 * m2.num13;
	result.num10 = m1.num8 * m2.num2 + m1.num9 * m2.num6 + m1.num10 * m2.num10 + m1.num11 * m2.num14;
	result.num11 = m1.num8 * m2.num3 + m1.num9 * m2.num7 + m1.num10 * m2.num11 + m1.num11 * m2.num15;
	result.num12 = m1.num12 * m2.num0 + m1.num13 * m2.num4 + m1.num14 * m2.num8 + m1.num15 * m2.num12;
	result.num13 = m1.num12 * m2.num1 + m1.num13 * m2.num5 + m1.num14 * m2.num9 + m1.num15 * m2.num13;
	result.num14 = m1.num12 * m2.num2 + m1.num13 * m2.num6 + m1.num14 * m2.num10 + m1.num15 * m2.num14;
	result.num15 = m1.num12 * m2.num3 + m1.num13 * m2.num7 + m1.num14 * m2.num11 + m1.num15 * m2.num15;

	return result;
}

DNMATH Matrix4 Matrix4Rotate(Vector3 axis, float angle)
{
	Matrix4 result = { 0 };

	float x = axis.x, y = axis.y, z = axis.z;

	float lengthSquared = x * x + y * y + z * z;

	if ((lengthSquared != 1.0f) && (lengthSquared != 0.0f))
	{
		float ilength = 1.0f / sqrtf(lengthSquared);
		x *= ilength;
		y *= ilength;
		z *= ilength;
	}

	float sinres = sinf(angle);
	float cosres = cosf(angle);
	float t = 1.0f - cosres;

	result.num0 = x * x * t + cosres;
	result.num1 = y * x * t + z * sinres;
	result.num2 = z * x * t - y * sinres;
	result.num3 = 0.0f;

	result.num4 = x * y * t - z * sinres;
	result.num5 = y * y * t + cosres;
	result.num6 = z * y * t + x * sinres;
	result.num7 = 0.0f;

	result.num8 = x * z * t + y * sinres;
	result.num9 = y * z * t - x * sinres;
	result.num10 = z * z * t + cosres;
	result.num11 = 0.0f;

	result.num12 = 0.0f;
	result.num13 = 0.0f;
	result.num14 = 0.0f;
	result.num15 = 1.0f;

	return result;
}

DNMATH Matrix4 Matrix4RotateX(float angle)
{
	Matrix4 result = { 0 };

	float sinres = sinf(angle);
	float cosres = cosf(angle);

	result.num0 = 1.0f;
	result.num5 = cosres;
	result.num6 = sinres;
	result.num9 = -sinres;
	result.num10 = cosres;
	result.num15 = 1.0f;

	return result;
}

DNMATH Matrix4 Matrix4RotateY(float angle)
{
	Matrix4 result = { 0 };

	float sinres = sinf(angle);
	float cosres = cosf(angle);

	result.num0 = cosres;
	result.num2 = -sinres;
	result.num5 = 1.0f;
	result.num8 = sinres;
	result.num10 = cosres;
	result.num15 = 1.0f;

	return result;
}

DNMATH Matrix4 Matrix4RotateZ(float angle)
{
	Matrix4 result = { 0 };

	float sinres = sinf(angle);
	float cosres = cosf(angle);

	result.num0 = cosres;
	result.num1 = sinres;
	result.num4 = -sinres;
	result.num5 = cosres;
	result.num10 = 1.0f;
	result.num15 = 1.0f;

	return result;
}

DNMATH Matrix4 Matrix4RotateXYZ(Vector3 angle)
{
	Matrix4 rx = Matrix4RotateX(angle.x);
	Matrix4 ry = Matrix4RotateY(angle.y);
	Matrix4 rz = Matrix4RotateZ(angle.z);

	return Matrix4Multiply(Matrix4Multiply(rz, ry), rx);
}

DNMATH Matrix4 Matrix4RotateZYX(Vector3 angle)
{
	Matrix4 rx = Matrix4RotateX(angle.x);
	Matrix4 ry = Matrix4RotateY(angle.y);
	Matrix4 rz = Matrix4RotateZ(angle.z);

	return Matrix4Multiply(Matrix4Multiply(rx, ry), rz);
}

DNMATH Matrix4 Matrix4Identity()
{
	Matrix4 result = { 0 };

	result.num0 = 1.0f;
	result.num5 = 1.0f;
	result.num10 = 1.0f;
	result.num15 = 1.0f;

	return result;
}

// Row-vector convention (v * M): translation sits in the last row (num12, num13, num14)

DNMATH Matrix4 Matrix4Translation(float tx, float ty, float tz)
{
	Matrix4 result = Matrix4Identity();

	result.num12 = tx;
	result.num13 = ty;
	result.num14 = tz;

	return result;
}

DNMATH Matrix4 Matrix4Scale(float sx, float sy, float sz)
{
	Matrix4 result = Matrix4Identity();

	result.num0 = sx;
	result.num5 = sy;
	result.num10 = sz;

	return result;
}

DNMATH Matrix4 Matrix4Perspective(float fovY, float aspect, float nearZ, float farZ)
{
	Matrix4 result = { 0 };

	float f = 1.0f / tanf(fovY * 0.5f);
	float range = farZ / (farZ - nearZ);

	result.num0 = f / aspect;
	result.num5 = f;
	result.num10 = range;
	result.num11 = 1.0f;
	result.num14 = -nearZ * range;

	return result;
}

DNMATH Matrix4 Matrix4LookAt(Vector3 eye, Vector3 center, Vector3 up) 
{
	// Row-vector convention (v * M)
	Vector3 f = Vector3Normalize(Vector3Subtract(center, eye)); // forward
	Vector3 s = Vector3Normalize(Vector3Cross(f, up));          // right (side)
	Vector3 u = Vector3Cross(s, f);                             // up

	Matrix4 result = { 0 };

	// Row 0
	result.num0 = s.x;
	result.num1 = s.y;
	result.num2 = s.z;
	result.num3 = 0.0f;

	// Row 1
	result.num4 = u.x;
	result.num5 = u.y;
	result.num6 = u.z;
	result.num7 = 0.0f;

	// Row 2
	result.num8  = -f.x;
	result.num9  = -f.y;
	result.num10 = -f.z;
	result.num11 = 0.0f;

	// Row 3 (translation row for row-vector convention)
	result.num12 = -Vector3Dot(s, eye);
	result.num13 = -Vector3Dot(u, eye);
	result.num14 =  Vector3Dot(f, eye);
	result.num15 = 1.0f;

	return result;
}

// Quaternion Functions 


DNMATH Vector4 QuatIdentity()
{
	return { 0.0f, 0.0f, 0.0f, 1.0f };
}

DNMATH Vector4 QuatFromAxisAngle(Vector3 axis, float angle)
{
	// Normalize the axis first so the resulting quaternion is unit-length
	Vector3 n = Vector3Normalize(axis);

	float half = angle * 0.5f;
	float sinHalf = sinf(half);

	return { n.x * sinHalf, n.y * sinHalf, n.z * sinHalf, cosf(half) };
}

DNMATH Vector4 QuatMultiply(Vector4 q1, Vector4 q2)
{
	// Hamilton product — applies q1 first, then q2
	return
	{
		q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y,
		q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x,
		q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w,
		q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z
	};
}

DNMATH Vector4 QuatNormalize(Vector4 q)
{
	return Vector4Normalize(q);
}

DNMATH Vector4 QuatConjugate(Vector4 q)
{
	return { -q.x, -q.y, -q.z, q.w };
}

DNMATH Vector4 QuatSlerp(Vector4 q1, Vector4 q2, float t)
{
	float dot = Vector4Dot(q1, q2);

	// If dot is negative, negate q2 to take the shorter arc
	if (dot < 0.0f)
	{
		q2.x = -q2.x; q2.y = -q2.y; q2.z = -q2.z; q2.w = -q2.w;
		dot = -dot;
	}

	if (dot > 0.9995f)
	{
		Vector4 result =
		{
			q1.x + t * (q2.x - q1.x),
			q1.y + t * (q2.y - q1.y),
			q1.z + t * (q2.z - q1.z),
			q1.w + t * (q2.w - q1.w)
		};
		return QuatNormalize(result);
	}

	float theta0 = acosf(dot);
	float theta = theta0 * t;
	float sinTheta = sinf(theta);
	float sinTheta0 = sinf(theta0);

	float s1 = cosf(theta) - dot * sinTheta / sinTheta0;
	float s2 = sinTheta / sinTheta0;

	return
	{
		s1 * q1.x + s2 * q2.x,
		s1 * q1.y + s2 * q2.y,
		s1 * q1.z + s2 * q2.z,
		s1 * q1.w + s2 * q2.w
	};
}

DNMATH Matrix4 QuatToMatrix4(Vector4 q)
{
	Matrix4 result = { 0 };

	float x = q.x, y = q.y, z = q.z, w = q.w;
	float x2 = x + x, y2 = y + y, z2 = z + z;

	float xx = x * x2;   float xy = x * y2;   float xz = x * z2;
	float yy = y * y2;   float yz = y * z2;   float zz = z * z2;
	float wx = w * x2;   float wy = w * y2;   float wz = w * z2;

	// Row 0
	result.num0 = 1.0f - (yy + zz);
	result.num1 = xy + wz;
	result.num2 = xz - wy;
	result.num3 = 0.0f;

	// Row 1
	result.num4 = xy - wz;
	result.num5 = 1.0f - (xx + zz);
	result.num6 = yz + wx;
	result.num7 = 0.0f;

	// Row 2
	result.num8 = xz + wy;
	result.num9 = yz - wx;
	result.num10 = 1.0f - (xx + yy);
	result.num11 = 0.0f;

	// Row 3 (translation row — none for a pure rotation)
	result.num12 = 0.0f;
	result.num13 = 0.0f;
	result.num14 = 0.0f;
	result.num15 = 1.0f;

	return result;
}

DNMATH Vector3 QuatRotateVector(Vector4 q, Vector3 v)
{

	Vector3 u = { q.x, q.y, q.z };
	float   s = q.w;

	// 2 * dot(u, v) * u
	float dot_uv = Vector3Dot(u, v);
	Vector3 term1 = Vector3Scale(u, 2.0f * dot_uv);

	// (s*s - dot(u,u)) * v
	float dot_uu = Vector3Dot(u, u);
	Vector3 term2 = Vector3Scale(v, s * s - dot_uu);

	// 2 * s * cross(u, v)
	Vector3 cross = Vector3Cross(u, v);
	Vector3 term3 = Vector3Scale(cross, 2.0f * s);

	return Vector3Add(Vector3Add(term1, term2), term3);
}


// Moduler Math Functions 


DNMATH float Fade(float t)
{
	return t * t * t * (t * (t * 6 - 15) + 10);
}

DNMATH float Grad(int hash, float x, float y)
{
	int h = hash & 3;
	float u = h < 2 ? x : y;
	float v = h < 2 ? y : x;
	return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

DNMATH float Lerp(float a, float b, float t)
{
	return a + t * (b - a);
}

DNMATH float Perlin(float x, float y, const int* perm)
{
	int xi = (int)std::floor(x) & 255;
	int yi = (int)std::floor(y) & 255;
	float xf = x - std::floor(x);
	float yf = y - std::floor(y);

	float u = Fade(xf);
	float v = Fade(yf);

	int aa = perm[perm[xi] + yi];
	int ab = perm[perm[xi] + yi + 1];
	int ba = perm[perm[xi + 1] + yi];
	int bb = perm[perm[xi + 1] + yi + 1];

	return Lerp(
		Lerp(Grad(aa, xf, yf), Grad(ba, xf - 1, yf), u),
		Lerp(Grad(ab, xf, yf - 1), Grad(bb, xf - 1, yf - 1), u),
		v
	);
}

DNMATH float Clamp(float value, float min, float max)
{
	float result = (value < min) ? min : value;

	if (result > max) result = max;

	return result;
}

DNMATH float Normalize(float value, float start, float end)
{
	float result = (value - start) / (end - start);

	return result;
}

DNMATH float Wrap(float value, float min, float max)
{
	float result = value - (max - min) * floorf((value - min) / (max - min));

	return result;
}

DNMATH float ReMap(float value, float inputStart, float inputEnd, float outputStart, float outputEnd)
{
	float result = (value - inputStart) / (inputEnd - inputStart) * (outputEnd - outputStart) + outputStart;

	return result;
}

DNMATH float FloatEqual(float x, float y)
{
	int result = (fabsf(x - y)) <= (EPSILON * fmaxf(1.0f, fmaxf(fabsf(x), fabsf(y))));

	return result;
}