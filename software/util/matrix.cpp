#include "util/matrix.h"
#include <cassert>
#include <sstream>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_permutation.h>

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

std::string Matrix::str() const {
	std::ostringstream oss;
	oss << *this;
	return oss.str();
}

Matrix &operator+=(Matrix &a, const Matrix &b) {
	assert(a.rows() == b.rows() && a.cols() == b.cols());
	if (a.m) {
		gsl_matrix_add(a.m, b.m);
	}
	return a;
}

Matrix &operator-=(Matrix &a, const Matrix &b) {
	assert(a.rows() == b.rows() && a.cols() == b.cols());
	if (a.m) {
		gsl_matrix_sub(a.m, b.m);
	}
	return a;
}

Matrix &operator*=(Matrix &m, double scale) {
	if (m.m) {
		gsl_matrix_scale(m.m, scale);
	}
	return m;
}

Matrix operator*(const Matrix &a, const Matrix &b) {
	assert(a.cols() == b.rows());
	Matrix prod(a.rows(), b.cols(), Matrix::ZEROES); // Even if beta = 0, uninitialized memory could contain NaN which will propagate.
	if (a.m) {
		gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, a.m, b.m, 0.0, prod.m);
	}
	return prod;
}

Matrix operator~(const Matrix &m) {
	Matrix trans(m.cols(), m.rows());
	if (m.m) {
		gsl_matrix_transpose_memcpy(trans.m, m.m);
	}
	return trans;
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

