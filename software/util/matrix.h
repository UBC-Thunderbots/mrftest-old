#ifndef UTIL_MATRIX_H
#define UTIL_MATRIX_H

#include <cstddef>
#include <ostream>
#include <string>
#include <gsl/gsl_matrix.h>

/**
 * A rectangular matrix.
 */
class Matrix {
	public:
		/**
		 * Flags for how to initialize new matrices.
		 */
		enum InitFlag {
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
		 * Adds some other matrix to this matrix.
		 *
		 * \param[in] b the matrix to add.
		 *
		 * \return this matrix.
		 */
		Matrix &operator+=(const Matrix &b);

		/**
		 * Adds two matrices.
		 *
		 * \param[in] b the second matrix to add.
		 *
		 * \return the sum of this matrix and \p b.
		 */
		Matrix operator+(const Matrix &b) const;

		/**
		 * Subtracts some other matrix from this matrix.
		 *
		 * \param[in] b the matrix to subtract.
		 *
		 * \return this matrix.
		 */
		Matrix &operator-=(const Matrix &b);

		/**
		 * Subtracts two matrices.
		 *
		 * \param[in] b the second matrix to subtract.
		 *
		 * \return the difference between this matrix and \p b.
		 */
		Matrix operator-(const Matrix &b) const;

		/**
		 * Multiplies this matrix by a scalar.
		 *
		 * \param[in] scale the scalar value to multiply by.
		 *
		 * \return this matrix.
		 */
		Matrix &operator*=(double scale);

		/**
		 * Multiplies a matrix by a scalar.
		 *
		 * \param[in] scale the scalar value to multiply by.
		 *
		 * \return the product of this matrix and \p scale.
		 */
		Matrix operator*(double scale) const;

		/**
		 * Multiplies this matrix by another matrix.
		 *
		 * \param[in] b the matrix to multiply by.
		 *
		 * \return this matrix.
		 *
		 * \pre <code>cols() == b.rows()</code>
		 */
		Matrix &operator*=(const Matrix &b);

		/**
		 * Multiplies two matrices.
		 *
		 * \param[in] b the second matrix to multiply.
		 *
		 * \return the product of this matrix and \p b.
		 *
		 * \pre <code>cols() == b.rows()</code>
		 */
		Matrix operator*(const Matrix &b) const;

		/**
		 * Divides this matrix by a scalar.
		 *
		 * \param[in] scale the scalar value to divide by.
		 *
		 * \return this matrix.
		 */
		Matrix &operator/=(double scale) {
			return *this *= 1.0 / scale;
		}

		/**
		 * Divides a matrix by a scalar.
		 *
		 * \param[in] scale the scalar value to divide by.
		 *
		 * \return the quotient of this matrix and \p scale.
		 */
		Matrix operator/(double scale) const {
			return *this * (1.0 / scale);
		}

		/**
		 * Copies data from an existing matrix into this matrix.
		 *
		 * \param[in] b the matrix to duplicate.
		 *
		 * \return this matrix.
		 */
		Matrix &operator=(const Matrix &b);

		/**
		 * Computes the transpose of this matrix.
		 *
		 * \return the transpose.
		 */
		Matrix operator~() const;

		/**
		 * Computes the inverse of this matrix.
		 *
		 * \return the inverse.
		 */
		Matrix operator!() const;

	private:
		gsl_matrix *m;
};

/**
 * Multiplies a matrix by a scalar.
 *
 * \param[in] scale the scalar value to multiply by.
 *
 * \param[in] m the matrix to multiply.
 *
 * \return \p scale × \p m.
 */
inline Matrix operator*(double scale, const Matrix &m) {
	return m * scale;
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

