#ifndef __GEOMETRY_H__
#define __GEOMETRY_H__

#include <iostream>
#include <cmath>
#include <cassert>

// �������
template<size_t DIM, typename T>
struct vec
{
	vec()
	{
		std::fill_n(m_data, DIM, T());
	}

	// ֧��ʹ�ó�ʼ���б�
	vec(std::initializer_list<T> list)
	{
		assert(list.size() == DIM);
		std::copy(list.begin(), list.end(), m_data);
	}

	vec(const vec& other)
	{
		std::copy(other.m_data, other.m_data + DIM, m_data);
	}

	vec& operator=(const vec& other)
	{
		if (this != &other)
			std::copy(other.m_data, other.m_data + DIM, m_data);

		return *this;
	}

	T& operator[](const size_t index)
	{
		assert(index < DIM);
		return m_data[index];
	}

	const T& operator[](const size_t index) const
	{
		assert(index < DIM);
		return m_data[index];
	}

private:
	T m_data[DIM];
};

template<typename T>
struct vec<2, T>
{
	vec() : x(T()), y(T()) {}
	vec(T x, T y) : x(x), y(y) {}

	template<typename U>
	vec(const vec<2, U>& other)
	{
		if constexpr (std::is_same_v<T, int> && std::is_same_v<U, float>)
		{
			x = static_cast<int>(std::round(other.x));
			y = static_cast<int>(std::round(other.y));
		}
		else
		{
			x = static_cast<T>(other.x);
			y = static_cast<T>(other.y);
		}
	}

	T& operator[](const size_t index)
	{
		assert(index < 2);
		return index <= 0 ? x : y;
	}

	const T& operator[](const size_t index) const
	{
		assert(index < 2);
		return index <= 0 ? x : y;
	}

	T x;
	T y;
};

template<typename T>
struct vec<3, T>
{
	vec() : x(T()), y(T()), z(T()) {}
	vec(T value) : x(value), y(value), z(value) {}
	vec(T x, T y, T z) : x(x), y(y), z(z) {}

	template<typename U>
	vec(const vec<3, U>& other)
	{
		if constexpr (std::is_same_v<T, int> && std::is_same_v<U, float>)
		{
			x = static_cast<int>(std::round(other.x));
			y = static_cast<int>(std::round(other.y));
			z = static_cast<int>(std::round(other.z));
		}
		else
		{
			x = static_cast<T>(other.x);
			y = static_cast<T>(other.y);
			z = static_cast<T>(other.z);
		}
	}

	vec(const vec<4, T>& other)
	{
		this->x = other[0];
		this->y = other[1];
		this->z = other[2];
	}

	T& operator[](const size_t index)
	{
		assert(index < 3);
		return (index == 0) ? x : (index == 1) ? y : z;
	}

	const T& operator[](const size_t index) const
	{
		assert(index < 3);
		return (index == 0) ? x : (index == 1) ? y : z;
	}

	vec<3, T>& normalize()
	{
		*this = (*this) * (1 / norm());
		return *this;
	}

	T x;
	T y;
	T z;

private:
	T norm() const
	{
		return std::sqrt(x * x + y * y + z * z);
	}
};

/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
/////////////////////////////////////////////////////////////////////////////////

// ��������
// ��ʾ���
template<size_t DIM, typename T>
T operator* (const vec<DIM, T>& left, const vec<DIM, T>& right)
{
	T ret = T();
	for (int index = 0; index < DIM; ++index)
	{
		ret += left[index] * right[index];
	}

	return ret;
}

template<size_t DIM, typename T>
T dot(const vec<DIM, T>& left, const vec<DIM, T>& right)
{
	return left * right;
}

// ��ӦԪ�����
template<size_t DIM, typename T>
vec<DIM, T> multiply_elements(const vec<DIM, T>& left, const vec<DIM, T>& right)
{
	vec<DIM, T> ret;
	for (int index = 0; index < DIM; ++index)
	{
		ret[index] = left[index] * right[index];
	}

	return ret;
}

template<size_t DIM, typename T>
vec<DIM, T> operator+ (const vec<DIM, T>& left, const vec<DIM, T>& right)
{
	vec<DIM, T> ret;
	for (int index = 0; index < DIM; ++index)
	{
		ret[index] = left[index] + right[index];
	}

	return ret;
}

template<size_t DIM, typename T>
vec<DIM, T> operator- (const vec<DIM, T>& left, const vec<DIM, T>& right)
{
	vec<DIM, T> ret;
	for (int index = 0; index < DIM; ++index)
	{
		ret[index] = left[index] - right[index];
	}

	return ret;
}

template<size_t DIM, typename T, typename U>
vec<DIM, T> operator* (const vec<DIM, T>& left, U coefficient)
{
	vec<DIM, T> ret;
	for (int index = 0; index < DIM; ++index)
	{
		ret[index] = left[index] * (T)coefficient;
	}

	return ret;
}

template<size_t DIM, typename T, typename U>
vec<DIM, T> operator/ (const vec<DIM, T>& left, U coefficient)
{
	vec<DIM, T> ret;
	for (int index = 0; index < DIM; ++index)
	{
		ret[index] = left[index] / coefficient;
	}

	return ret;
}

template<size_t LEN, size_t DIM, typename T>
vec<DIM, T> proj(const vec<DIM, T>& src)
{
	assert(LEN <= DIM);

	vec<LEN, T> des;
	for (int index = 0; index < LEN; ++index)
	{
		des[index] = src[index];
	}

	return des;
}

template <typename T>
vec<3, T> cross(const vec<3, T>& v1, const vec<3, T>& v2)
{
	return vec<3, T>(
		v1.y * v2.z - v1.z * v2.y,  // x ����
		v1.z * v2.x - v1.x * v2.z,  // y ����
		v1.x * v2.y - v1.y * v2.x   // z ����
	);
}

template<size_t DIM, typename T>
std::iostream& operator<< (std::iostream& out, const vec<DIM, T>& left)
{
	for (int index = 0; index < DIM; ++index)
	{
		out << left[index] << " "
	}

	return out;
}

/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
/////////////////////////////////////////////////////////////////////////////////

template<size_t ROWS, size_t COLS, typename T> class mat;

// ��������ʽ
template<size_t DIM, typename T>
struct dt
{
	static T det(const mat<DIM, DIM, T>& src)
	{
		T ret = 0;
		for (size_t i = DIM; i--; ret += src[0][i] * src.cofactor(0, i));
		return ret;
	}
};

template<typename T>
struct dt<1, T>
{
	static T det(const mat<1, 1, T>& src)
	{
		return src[0][0];
	}
};

/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
/////////////////////////////////////////////////////////////////////////////////

// ����
template<size_t ROWS, size_t COLS, typename T>
class mat
{
public:
	~mat() = default;
	mat()
	{
		if constexpr (ROWS == COLS)
		{
			for (int col = 0; col < COLS; ++col)
			{
				m_data[col][col] = 1;
			}
		}
	}

	// ����������
	vec<COLS, T>& operator[] (int index)
	{
		assert(index < ROWS);
		return m_data[index];
	}

	const vec<COLS, T>& operator[] (int index) const
	{
		assert(index < ROWS);
		return m_data[index];
	}

	// ����������
	vec<ROWS, T> col(int index) const
	{
		assert(index < COLS);
		vec<ROWS, T> ret;
		for (int i = 0; i < ROWS; ++i)
		{
			ret[i] = m_data[i][index];
		}

		return ret;
	}

	// ����������
	void setCol(int index, const vec<ROWS, T>& src)
	{
		assert(index < COLS);
		for (int i = 0; i < ROWS; ++i)
		{
			m_data[i][index] = src[i];
		}
	}

	// ��ȡ���Ӿ������Ϊɾ��ĳ��ĳ��
	mat<ROWS - 1, COLS - 1, T> get_minor(size_t row, size_t col) const
	{
		mat<ROWS - 1, COLS - 1, T> ret;
		for (size_t i = ROWS - 1; i--; )
			for (size_t j = COLS - 1; j--; ret[i][j] = m_data[i < row ? i : i + 1][j < col ? j : j + 1]);
		return ret;
	}

	T det() const
	{
		return dt<COLS, T>::det(*this);
	}

	// ��ȡ����ʽ
	T cofactor(size_t row, size_t col) const
	{
		return get_minor(row, col).det() * ((row + col) % 2 ? -1 : 1);
	}

	// �������
	mat<ROWS, COLS, T> adjugate() const
	{
		mat<ROWS, COLS, T> ret;
		for (size_t i = ROWS; i--; )
			for (size_t j = COLS; j--; ret[i][j] = cofactor(i, j));
		return ret;
	}

	// �����
	mat<ROWS, COLS, T> invert_transpose()
	{
		assert(ROWS == COLS);
		mat<ROWS, COLS, T> ret = adjugate();
		T tmp = ret[0] * m_data[0];
		assert(tmp != 0);
		return ret / tmp;
	}

	mat<ROWS, COLS, T> invert() const
	{
		assert(ROWS == COLS);

		// ��ʼ��һ����������ԭ���󣩺͵�λ���������
		mat<ROWS, COLS, T> srcMatrix = *this;
		mat<ROWS, COLS, T> inverse;

		// ��˹-Լ������Ԫ��
		for (size_t i = 0; i < ROWS; ++i)
		{
			// Ѱ�ҵ�ǰ���о���ֵ������Ԫ
			size_t pivotRow = i;
			T pivotValue = srcMatrix[i][i];
			for (size_t k = i + 1; k < ROWS; ++k)
			{
				if (std::abs(srcMatrix[k][i]) > std::abs(pivotValue))
				{
					pivotValue = srcMatrix[k][i];
					pivotRow = k;
				}
			}

			// �����ԪΪ�㣬����󲻿���
			assert(pivotValue != 0);

			// ������ǰ�к���Ԫ��
			if (pivotRow != i)
			{
				std::swap(srcMatrix[i], srcMatrix[pivotRow]);
				std::swap(inverse[i], inverse[pivotRow]);
			}

			// ��һ����ǰ�У�ʹ��Ԫ��Ϊ1��
			T pivot = srcMatrix[i][i];
			for (size_t j = 0; j < ROWS; ++j)
			{
				srcMatrix[i][j] /= pivot;
				inverse[i][j] /= pivot;
			}

			// ��ȥ�����еĵ�ǰ��
			for (size_t j = 0; j < ROWS; ++j)
			{
				if (i != j)
				{
					T factor = srcMatrix[j][i];
					for (size_t k = 0; k < ROWS; ++k)
					{
						srcMatrix[j][k] -= factor * srcMatrix[i][k];
						inverse[j][k] -= factor * inverse[i][k];
					}
				}
			}
		}

		return inverse;
	}

private:
	vec<COLS, T> m_data[ROWS];
};

/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
/////////////////////////////////////////////////////////////////////////////////

 // �������
template<size_t ROWS, size_t COLS, typename T>
vec<ROWS, T> operator* (const mat<ROWS, COLS, T>& matrix, const vec<COLS, T>& vector)
{
	vec<ROWS, T> ret;
	for (int row = 0; row < ROWS; ++row)
	{
		ret[row] = matrix[row] * vector;
	}

	return ret;
}

template<size_t M, size_t N, size_t K, typename T>
mat<M, K, T> operator* (const mat<M, N, T>& left, const mat<N, K, T>& right)
{
	mat<M, K, T> ret;
	for (int row = 0; row < M; ++row)
	{
		for (int col = 0; col < K; ++col)
		{
			ret[row][col] = left[row] * right.col(col);
		}
	}

	return ret;
}

template<size_t DimRows, size_t DimCols, typename T>
mat<DimCols, DimRows, T> operator/(mat<DimRows, DimCols, T> lhs, const T& rhs)
{
	for (size_t i = DimRows; i--; lhs[i] = lhs[i] / rhs);
	return lhs;
}

template<size_t DimRows, size_t DimCols, typename T>
mat<DimCols, DimRows, T> operator*(mat<DimRows, DimCols, T> lhs, const T& rhs)
{
	for (size_t i = DimRows; i--; lhs[i] = lhs[i] * rhs);
	return lhs;
}

template <size_t DimRows, size_t DimCols, class T> 
std::ostream& operator<<(std::ostream& out, mat<DimRows, DimCols, T>& m) 
{
	for (size_t i = 0; i < DimRows; i++) out << m[i] << std::endl;
	return out;
}

/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
/////////////////////////////////////////////////////////////////////////////////

using Vec2f = vec<2, float>;
using Vec2i = vec<2, int>;
using Vec3f = vec<3, float>;
using Vec3i = vec<3, int>;
using Vec4f = vec<4, float>;
using Matrix = mat<4, 4, float>;

#endif // __GEOMETRY_H__