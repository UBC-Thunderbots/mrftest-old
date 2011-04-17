#ifndef UTIL_MATRIX_H
#define UTIL_MATRIX_H

#include <cstddef>
#include <ostream>
#include <string>
#include <utility>
#include <gsl/gsl_matrix.h>

/**
 * A rectangular matrix.
 */
class Matrix {
	public:
		/**
		 * Flags for how to initialize new matrices.
		 */
		enum class InitFlag {
			/**
			 * Indicates that a newly-allocated matrix should be zeroed.
			 */
			ZEROES,

			/**
			 * Indicates that a newly-allocated matrix should be set to the identity matrix.
			 */
			IDENTITY,
		};

		/**
		 * Constructs a zero-by-zero matrix.
		 */
		Matrix();

		/**
		 * Constructs a matrix.
		 *
		 * \param[in] num_rows the number of rows in the new matrix.
		 *
		 * \param[in] num_cols the number of columns in the new matrix (defaults to 1 for a column vector).
		 *
		 * \param[in] data the data to fill in the the matrix with, in row-major order (defaults to 0 for an uninitialized matrix).
		 */
		Matrix(std::size_t num_rows, std::size_t num_cols = 1, const double *data = 0);

		/**
		 * Constructs a matrix initialized to a common pattern.
		 *
		 * \param[in] num_rows the number of rows in the new matrix.
		 *
		 * \param[in] num_cols the number of columns in the new matrix (defaults to 1 for a column vector).
		 *
		 * \param[in] flag the pattern to fill with.
		 */
		Matrix(std::size_t num_rows, std::size_t num_cols, InitFlag flag);

		/**
		 * Constructs a copy of an existing matrix.
		 *
		 * \param[in] copyref the matrix to duplicate.
		 */
		Matrix(const Matrix &copyref);

		/**
		 * Moves an existing matrix into a new matrix.
		 *
		 * \param[in] moveref the matrix to move.
		 */
		Matrix(Matrix &&moveref);

		/**
		 * Destroys a matrix.
		 */
		~Matrix();

		/**
		 * Returns the number of rows in the matrix.
		 *
		 * \return the number of rows in the matrix.
		 */
		std::size_t rows() const {
			return m->size1;
		}

		/**
		 * Returns the number of columns in the matrix.
		 *
		 * \return the number of columns in the matrix.
		 */
		std::size_t cols() const {
			return m->size2;
		}

		/**
		 * Sets this matrix to be filled with zeroes.
		 */
		void zero();

		/**
		 * Sets this matrix to be the identity matrix.
		 */
		void identity();

		/**
		 * Sets this matrix to be its own transpose.
		 */
		void transpose();

		/**
		 * Sets this matrix to be its own inverse.
		 */
		void invert();

		/**
		 * Swaps this matrix with another matrix.
		 *
		 * \param[in] b the matrix to swap with.
		 */
		void swap(Matrix &b);

		/**
		 * Returns a string representation of this matrix.
		 *
		 * \return a string representation of this matrix.
		 */
		std::string str() const;

		/**
		 * Accesses an element of the matrix.
		 *
		 * \param[in] row the row of the element to return.
		 *
		 * \param[in] col the column of the element to return.
		 *
		 * \return the element.
		 */
		double &operator()(std::size_t row, std::size_t col) {
			return m->data[row * m->tda + col];
		}

		/**
		 * Accesses an element of the matrix.
		 *
		 * \param[in] row the row of the element to return.
		 *
		 * \param[in] col the column of the element to return.
		 *
		 * \return the element.
		 */
		double operator()(std::size_t row, std::size_t col) const {
			return m->data[row * m->tda + col];
		}

		/**
		 * Copies data from an existing matrix into this matrix.
		 *
		 * \param[in] b the matrix to duplicate.
		 *
		 * \return this matrix.
		 */
		Matrix &operator=(Matrix b);

	private:
		gsl_matrix *m;

		friend Matrix &operator+=(Matrix &, const Matrix &);
		friend Matrix &operator-=(Matrix &, const Matrix &);
		friend Matrix &operator*=(Matrix &, double);
		friend Matrix operator*(const Matrix &a, const Matrix &b);
		friend Matrix operator~(const Matrix &m);
};

inline Matrix::Matrix() : m(0) {
}

inline Matrix::Matrix(Matrix &&moveref) : m(moveref.m) {
	moveref.m = 0;
}

inline void Matrix::swap(Matrix &b) {
	std::swap(m, b.m);
}

inline Matrix &Matrix::operator=(Matrix b) {
	swap(b);
	return *this;
}

/**
 * Adds a matrix to another matrix.
 *
 * \param[in] a the matrix to add to.
 *
 * \param[in] b the matrix to add.
 *
 * \return \p a.
 */
Matrix &operator+=(Matrix &a, const Matrix &b);

/**
 * Subtracts a matrix from another matrix.
 *
 * \param[in] a the matrix to subtract from.
 *
 * \param[in] b the matrix to subtract.
 *
 * \return \p a.
 */
Matrix &operator-=(Matrix &a, const Matrix &b);

/**
 * Multiplies this matrix by a scalar.
 *
 * \param[in] m the matrix to scale.
 *
 * \param[in] scale the scalar value to multiply by.
 *
 * \return \p m.
 */
Matrix &operator*=(Matrix &m, double scale);

/**
 * Multiplies two matrices.
 *
 * \param[in] a the first matrix to multiply.
 *
 * \param[in] b the second matrix to multiply.
 *
 * \return the product of \p a and \p b.
 *
 * \pre <code>a.cols() == b.rows()</code>
 */
Matrix operator*(const Matrix &a, const Matrix &b);

/**
 * Computes the transpose of this matrix.
 *
 * \param[in] m the matrix to transpose.
 *
 * \return the transpose.
 */
Matrix operator~(const Matrix &m);

/**
 * Adds two matrices.
 *
 * \param[in] a the first matrix to add.
 *
 * \param[in] b the second matrix to add.
 *
 * \return the sum of \p a and \p b.
 */
inline Matrix operator+(const Matrix &a, const Matrix &b) {
	Matrix temp(a);
	temp += b;
	return temp;
}

/**
 * Adds two matrices.
 *
 * \param[in] a the first matrix to add.
 *
 * \param[in] b the second matrix to add.
 *
 * \return the sum of \p a and \p b.
 */
inline Matrix operator+(Matrix &&a, const Matrix &b) {
	return std::move(a += b);
}

/**
 * Adds two matrices.
 *
 * \param[in] a the first matrix to add.
 *
 * \param[in] b the second matrix to add.
 *
 * \return the sum of \p a and \p b.
 */
inline Matrix operator+(const Matrix &a, Matrix &&b) {
	return std::move(b += a);
}

/**
 * Adds two matrices.
 *
 * \param[in] a the first matrix to add.
 *
 * \param[in] b the second matrix to add.
 *
 * \return the sum of \p a and \p b.
 */
inline Matrix operator+(Matrix &&a, Matrix &&b) {
	return std::move(a += b);
}

/**
 * Subtracts two matrices.
 *
 * \param[in] a the matrix to subtract from.
 *
 * \param[in] b the matrix to subtract.
 *
 * \return the difference between \p a and \p b.
 */
inline Matrix operator-(const Matrix &a, const Matrix &b) {
	Matrix temp(a);
	temp -= b;
	return temp;
}

/**
 * Subtracts two matrices.
 *
 * \param[in] a the matrix to subtract from.
 *
 * \param[in] b the matrix to subtract.
 *
 * \return the difference between \p a and \p b.
 */
inline Matrix operator-(Matrix &&a, const Matrix &b) {
	return std::move(a -= b);
}

/**
 * Multiplies a matrix by a scalar.
 *
 * \param[in] m the matrix to scale.
 *
 * \param[in] scale the scalar value to multiply by.
 *
 * \return the product of \p m and \p scale.
 */
inline Matrix operator*(const Matrix &m, double scale) {
	Matrix temp(m);
	temp *= scale;
	return temp;
}

/**
 * Multiplies a matrix by a scalar.
 *
 * \param[in] m the matrix to scale.
 *
 * \param[in] scale the scalar value to multiply by.
 *
 * \return the product of \p m and \p scale.
 */
inline Matrix operator*(Matrix &&m, double scale) {
	return std::move(m *= scale);
}

/**
 * Multiplies a matrix by a scalar.
 *
 * \param[in] scale the scalar value to multiply by.
 *
 * \param[in] m the matrix to scale.
 *
 * \return the product of \p m and \p scale.
 */
inline Matrix operator*(double scale, const Matrix &m) {
	Matrix temp(m);
	temp *= scale;
	return temp;
}

/**
 * Multiplies a matrix by a scalar.
 *
 * \param[in] scale the scalar value to multiply by.
 *
 * \param[in] m the matrix to scale.
 *
 * \return the product of \p m and \p scale.
 */
inline Matrix operator*(double scale, Matrix &&m) {
	return std::move(m *= scale);
}

/**
 * Multiplies two matrices.
 *
 * \param[in] a the first matrix to multiply.
 *
 * \param[in] b the second matrix to multiply.
 *
 * \return \p a.
 *
 * \pre <code>cols() == b.rows()</code>
 */
inline Matrix &operator*=(Matrix &a, const Matrix &b) {
	Matrix temp = a * b;
	temp.swap(a);
	return a;
}

/**
 * Divides this matrix by a scalar.
 *
 * \param[in] m the matrix to divide.
 *
 * \param[in] scale the scalar value to divide by.
 *
 * \return \p m.
 */
inline Matrix &operator/=(Matrix &m, double scale) {
	return m *= 1.0 / scale;
}

/**
 * Divides a matrix by a scalar.
 *
 * \param[in] m the matrix to divide.
 *
 * \param[in] scale the scalar value to divide by.
 *
 * \return the quotient of \p m and \p scale.
 */
inline Matrix operator/(const Matrix &m, double scale) {
	return m * (1.0 / scale);
}

/**
 * Divides a matrix by a scalar.
 *
 * \param[in] m the matrix to divide.
 *
 * \param[in] scale the scalar value to divide by.
 *
 * \return the quotient of \p m and \p scale.
 */
inline Matrix operator/(Matrix &&m, double scale) {
	return std::forward<Matrix>(m) * (1.0 / scale);
}

/**
 * Computes the inverse of a matrix.
 *
 * \param[in] m the matrix.
 *
 * \return the inverse.
 */
inline Matrix operator!(const Matrix &m) {
	Matrix temp(m);
	temp.invert();
	return temp;
}

/**
 * Computes the inverse of a matrix.
 *
 * \param[in] m the matrix.
 *
 * \return the inverse.
 */
inline Matrix operator!(Matrix &&m) {
	m.invert();
	return std::move(m);
}

/**
 * Writes a text representation of a matrix to a stream.
 *
 * \param[in] stream the stream to write to.
 *
 * \return the stream.
 */
std::ostream &operator<<(std::ostream &stream, const Matrix &m);

namespace std {
	/**
	 * Swaps two matrices.
	 *
	 * \param[in, out] a the first matrix.
	 *
	 * \param[in, out] b the second matrix.
	 */
	inline void swap(Matrix &a, Matrix &b) {
		a.swap(b);
	}
}

#endif

