#include "test_ls_matrix.hxx"

#include <simgear/constants.h>
#include <simgear/misc/test_macros.hxx>
extern "C" {
#include "src/FDM/LaRCsim/ls_matrix.h"
}


void LaRCSimMatrixTests::testCopyMatrix()
{
    int nelm = 20;
    double **src = nr_matrix(1, nelm, 1, nelm);
    double **dest = nr_matrix(1, nelm, 1, nelm);
    double invmaxlong = 1.0/(double)RAND_MAX;

    for (int i=1; i<=nelm; ++i)
        for (int j=1; j<=nelm; ++j)
            src[i][j] = 2.0 - 4.0*invmaxlong*(double) rand();

    nr_copymat(src, nelm, dest);

    for (int i=1; i<=nelm; ++i)
        for (int j=1; j<=nelm; ++j)
            CPPUNIT_ASSERT_EQUAL(src[i][j], dest[i][j]);

    nr_free_matrix(src, 1, nelm, 1, nelm);
    nr_free_matrix(dest, 1, nelm, 1, nelm);
}

void LaRCSimMatrixTests::testIdentityMatrix()
{
    int nelm = 10;
    double **id = nr_matrix(1, nelm, 1, nelm);

    for (int i=1; i<=nelm; ++i)
        for (int j=1; j<=nelm; ++j)
            id[i][j] = i == j ? 1.0 : 0.0;

    nr_gaussj(id, nelm, 0, 0);

    for (int i=1; i<=nelm; ++i)
        for (int j=1; j<=nelm; ++j)
            CPPUNIT_ASSERT_DOUBLES_EQUAL(id[i][j], i == j ? 1.0 : 0.0, 1e-9);

    nr_free_matrix(id, 1, nelm, 1, nelm);
}

void LaRCSimMatrixTests::testOrthogonalMatrix()
{
    int nelm = 3;
    double **m = nr_matrix(1, nelm, 1, nelm);
    double **inv = nr_matrix(1, nelm, 1, nelm);
    double angle = M_PI/3.0;

    m[1][1] = cos(angle);
    m[1][2] = sin(angle);
    m[1][3] = 0.0;
    m[2][1] = -m[1][2];
    m[2][2] = m[1][1];
    m[2][3] = 0.0;
    m[3][1] = 0.0;
    m[3][2] = 0.0;
    m[3][3] = 1.0;

    nr_copymat(m, nelm, inv);
    nr_gaussj(inv, nelm, 0, 0);

    for (int i=1; i<=nelm; ++i)
        for (int j=1; j<=nelm; ++j)
            CPPUNIT_ASSERT_DOUBLES_EQUAL(m[i][j], inv[j][i], 1e-9);

    nr_free_matrix(m, 1, nelm, 1, nelm);
    nr_free_matrix(inv, 1, nelm, 1, nelm);
}

void LaRCSimMatrixTests::testRandomMatrix()
{
    int nelm = 20;
    double **src = nr_matrix(1, nelm, 1, nelm);
    double **inv = nr_matrix(1, nelm, 1, nelm);
    double **id = nr_matrix(1, nelm, 1, nelm);
    double invmaxlong = 1.0/(double)RAND_MAX;

    for (int i=1; i<=nelm; ++i)
        for (int j=1; j<=nelm; ++j)
            src[i][j] = 2.0 - 4.0*invmaxlong*(double) rand();

    nr_copymat(src, nelm, inv);
    nr_gaussj(inv, nelm, 0, 0);

    for (int i=1; i<=nelm; ++i)
        for (int j=1; j<=nelm; ++j) {
            id[i][j] = 0.0;
            for (int k=1; k<=nelm; ++k)
                id[i][j] += src[i][k]*inv[k][j];
        }

    for (int i=1; i<=nelm; ++i)
        for (int j=1; j<=nelm; ++j)
            CPPUNIT_ASSERT_DOUBLES_EQUAL(id[i][j], i == j ? 1.0 : 0.0, 1e-9);

    nr_free_matrix(src, 1, nelm, 1, nelm);
    nr_free_matrix(inv, 1, nelm, 1, nelm);
    nr_free_matrix(id, 1, nelm, 1, nelm);
}

void LaRCSimMatrixTests::testSolveLinearSystem()
{
    int nelm = 20;
    double **src = nr_matrix(1, nelm, 1, nelm);
    double **inv = nr_matrix(1, nelm, 1, nelm);
    double **rhs = nr_matrix(1, nelm, 1, 1);
    double **sol = nr_matrix(1, nelm, 1, 1);
    double **check = nr_matrix(1, nelm, 1, nelm);
    double invmaxlong = 1.0/(double)RAND_MAX;

    for (int i=1; i<=nelm; ++i)
        for (int j=1; j<=nelm; ++j)
            src[i][j] = 2.0 - 4.0*invmaxlong*(double) rand();

    for (int i=1; i<=nelm; ++i) {
        rhs[i][1] = 2.0+cos(i*M_PI/nelm);
        sol[i][1] = rhs[i][1];
    }

    nr_copymat(src, nelm, inv);
    nr_gaussj(inv, nelm, sol, 1);

    for (int i=1; i<=nelm; ++i)
        for (int j=1; j<=nelm; ++j) {
            check[i][j] = 0.0;
            for (int k=1; k<=nelm; ++k)
                check[i][j] += src[i][k]*inv[k][j];
        }

    for (int i=1; i<=nelm; ++i)
        for (int j=1; j<=nelm; ++j)
            CPPUNIT_ASSERT_DOUBLES_EQUAL(check[i][j], i == j ? 1.0 : 0.0, 1e-9);

    for (int i=1; i<=nelm; ++i) {
        check[i][1] = 0.0;
        for (int j=1; j<=nelm; ++j)
            check[i][1] += src[i][j]*sol[j][1];
    }

    for (int i=1; i<=nelm; ++i)
        CPPUNIT_ASSERT_DOUBLES_EQUAL(check[i][1], rhs[i][1], 1e-9);

    nr_free_matrix(src, 1, nelm, 1, nelm);
    nr_free_matrix(inv, 1, nelm, 1, nelm);
    nr_free_matrix(check, 1, nelm, 1, nelm);
    nr_free_matrix(rhs, 1, nelm, 1, 1);
    nr_free_matrix(sol, 1, nelm, 1, 1);
}
