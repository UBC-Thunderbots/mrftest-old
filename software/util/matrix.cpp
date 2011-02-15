#include <cassert>
#include <sstream>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_permutation.h>
#include "util/matrix.h"

Matrix::Matrix() : m(0) {
}

Matrix::Matrix(std::size_t num_rows, std::size_t num_cols, const double *data) {
	if (num_rows && num_cols) {
		m = gsl_matrix_alloc(num_rows, num_cols);
		if (data) {
			for (std::size_t i = 0; i != num_rows; ++i) {
				for (std::size_t j = 0; j != num_cols; ++j) {
					m->data[i * m->tda + j] = data[i * num_rows + j];
				}
			}
		}
	} else {
		m = 0;
	}
}

Matrix::Matrix(std::size_t num_rows, std::size_t num_cols, InitFlag flag) {
	if (num_rows && num_cols) {
		switch (flag) {
			case ZEROES:
				m = gsl_matrix_calloc(num_rows, num_cols);
				break;

			case IDENTITY:
				m = gsl_matrix_alloc(num_rows, num_cols);
				identity();
				break;
		}
	} else {
		m = 0;
	}
}

Matrix::Matrix(const Matrix &copyref) {
	if (copyref.m) {
		m = gsl_matrix_alloc(copyref.m->size1, copyref.m->size2);
		gsl_matrix_memcpy(m, copyref.m);
	} else {
		m = 0;
	}
}

Matrix::~Matrix() {
	if (m) {
		gsl_matrix_free(m);
	}
}

void Matrix::zero() {
	if (m) {
		gsl_matrix_set_zero(m);
	}
}

void Matrix::identity() {
	if (m) {
		gsl_matrix_set_identity(m);
	}
}

void Matrix::transpose() {
	if (m) {
		if (rows() == cols()) {
			gsl_matrix_transpose(m);
		} else {
			Matrix temp = ~*this;
			swap(temp);
		}
	}
}

void Matrix::invert() {
	assert(rows() == cols());

	if (m) {
		Matrix temp(rows(), cols());
		gsl_permutation *perm = gsl_permutation_alloc(rows());
		int signum;
		gsl_linalg_LU_decomp(m, perm, &signum);
		gsl_linalg_LU_invert(m, perm, temp.m);
		gsl_permutation_free(perm);
		swap(temp);
	}
}

void Matrix::swap(Matrix &b) {
	std::swap(m, b.m);
}

std::string Matrix::str() const {
	std::ostringstream oss;
	oss << *this;
	return oss.str();
}

Matrix &Matrix::operator+=(const Matrix &b) {
	assert(rows() == b.rows() && cols() == b.cols());
	if (m) {
		gsl_matrix_add(m, b.m);
	}
	return *this;
}

Matrix Matrix::operator+(const Matrix &b) const {
	assert(rows() == b.rows() && cols() == b.cols());
	Matrix sum(*this);
	sum += b;
	return sum;
}

Matrix &Matrix::operator-=(const Matrix &b) {
	assert(rows() == b.rows() && cols() == b.cols());
	if (m) {
		gsl_matrix_sub(m, b.m);
	}
	return *this;
}

Matrix Matrix::operator-(const Matrix &b) const {
	assert(rows() == b.rows() && cols() == b.cols());
	Matrix diff(*this);
	diff -= b;
	return diff;
}

Matrix &Matrix::operator*=(double scale) {
	if (m) {
		gsl_matrix_scale(m, scale);
	}
	return *this;
}

Matrix Matrix::operator*(double scale) const {
	Matrix prod(*this);
	prod *= scale;
	return prod;
}

Matrix &Matrix::operator*=(const Matrix &b) {
	Matrix prod = *this * b;
	swap(prod);
	return *this;
}

Matrix Matrix::operator*(const Matrix &b) const {
	assert(cols() == b.rows());
	Matrix prod(rows(), b.cols(), ZEROES); // Even if beta = 0, uninitialized memory could contain NaN which will propagate.
	if (m) {
		gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, m, b.m, 0.0, prod.m);
	}
	return prod;
}

Matrix &Matrix::operator=(const Matrix &b) {
	Matrix temp(b);
	swap(temp);
	return *this;
}

Matrix Matrix::operator~() const {
	Matrix trans(cols(), rows());
	if (m) {
		gsl_matrix_transpose_memcpy(trans.m, m);
	}
	return trans;
}

Matrix Matrix::operator!() const {
	Matrix temp(*this);
	temp.invert();
	return temp;
}

std::ostream &operator<<(std::ostream &stream, const Matrix &m) {
	stream << '[';
	for (std::size_t i = 0; i < m.rows(); ++i) {
		if (i) {
			stream << ", ";
		}
		for (std::size_t j = 0; j < m.cols(); ++j) {
			if (j) {
				stream << ' ';
			}
			stream << m(i, j);
		}
	}
	stream << ']';
	return stream;
}

