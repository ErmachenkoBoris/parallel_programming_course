  // Copyright 2019 Volkov Artem

#include <tbb/tbb.h>
#include <omp.h>
#include <iostream>
#include <cstring>
#include <cmath>
#include <iomanip>
#include <cstdlib>
#include <vector>
#include <algorithm>


#define FIRST_CMT 1
#define SECOND_CMT 2
#define MAX_SIZE_DENSE_MATRIX 13000
#define CHUNK 10

#define MAX_VAL 100

struct CRSMatrix {
    int Size;
    int NonzeroSize;
    double* Values;
    int* Columns;
    int* RowIndex;
};

class Multiplicator {
    CRSMatrix CMT_1;
    CRSMatrix TCMT_2;
    std::vector<double>* Values;
    std::vector<int>* Columns;
    int* Rows;

 public:
    Multiplicator(const CRSMatrix& _CMT_1,
        const CRSMatrix& _TCMT_2,
    std::vector<double>* _Values, std::vector<int>* _Columns,
    int* _Rows) :
        CMT_1(_CMT_1), TCMT_2(_TCMT_2), Values(_Values),
    Columns(_Columns), Rows(_Rows)
    { }

    void operator()(const tbb::blocked_range<int>& r) const {
        int begin = r.begin();
        int end = r.end();
        int Size = CMT_1.Size;
        for (int i = begin; i < end; i++) {
            for (int j = 0; j < Size; j++) {
                double ScalarMult = 0;

                int ks = CMT_1.RowIndex[i];
                int ls = TCMT_2.RowIndex[j];
                int kf = CMT_1.RowIndex[i + 1] - 1;
                int lf = TCMT_2.RowIndex[j + 1] - 1;

                while ((ks <= kf) && (ls <= lf)) {
                    if (CMT_1.Columns[ks] <
                        TCMT_2.Columns[ls]) {
                        ks++;
                    } else {
                        if (CMT_1.Columns[ks] >
                            TCMT_2.Columns[ls]) {
                            ls++;
                        } else {
                            ScalarMult += CMT_1.Values[ks] *
                                TCMT_2.Values[ls];
                            ks++;
                            ls++;
                        }
                    }
                }

                if (ScalarMult != 0) {
                    Columns[i].push_back(j);
                    Values[i].push_back(ScalarMult);
                    Rows[i]++;
                }
            }
        }
    }
};

void CreateCRSMatrix(int Size, int NonzeroSize, CRSMatrix* CMT) {
    CMT->Size = Size;
    CMT->NonzeroSize = NonzeroSize;

    CMT->Values = new double[NonzeroSize];
    CMT->Columns = new int[NonzeroSize];
    CMT->RowIndex = new int[Size + 1];
}

void DeleteCRSMatrix(CRSMatrix* CMT) {
    delete[] CMT->Values;
    delete[] CMT->Columns;
    delete[] CMT->RowIndex;
}

void InitCRSMatr(int NumOfMatr, int Size, int NumElInRow,
    CRSMatrix* CMT) {
    int NonzeroSize, tmp;
    bool flag = false;

    NonzeroSize = NumElInRow * Size;
    srand(NumOfMatr);

    CreateCRSMatrix(Size, NonzeroSize, CMT);

    for (int i = 0; i < Size; i++) {
        for (int j = 0; j < NumElInRow; j++) {
            do {
                flag = false;
                CMT->Columns[i * NumElInRow + j] = std::rand()
                    % Size;
                for (int k = 0; k < j; k++) {
                    if (CMT->Columns[i * NumElInRow + j] ==
                        CMT->Columns[i * NumElInRow + k]) {
                        flag = true;
                    }
                }
            } while (flag == true);
        }

        for (int j = 0; j < NumElInRow - 1; j++) {
            for (int k = 0; k < NumElInRow - 1; k++) {
                if (CMT->Columns[i * NumElInRow + k] >
                    CMT->Columns[i * NumElInRow + k + 1]) {
                    tmp = CMT->Columns[i * NumElInRow + k];
                    CMT->Columns[i * NumElInRow + k] =
                        CMT->Columns[i * NumElInRow + k + 1];
                    CMT->Columns[i * NumElInRow + k + 1] = tmp;
                }
            }
        }
    }

    for (int i = 0; i < NonzeroSize; i++) {
        CMT->Values[i] = std::rand() % 100 - 50;

        if (CMT->Values[i] == 0) {
            CMT->Values[i] += 1;
        }
    }

    CMT->RowIndex[0] = 0;
    for (int i = 1; i < Size + 1; i++) {
        CMT->RowIndex[i] = CMT->RowIndex[i - 1] + NumElInRow;
    }
}


CRSMatrix Transposing(CRSMatrix CMT) {
    CRSMatrix TCMT;
    int TSize = CMT.Size;
    int TNonzeroSize = CMT.NonzeroSize;

    CreateCRSMatrix(TSize, TNonzeroSize, &TCMT);

    memset(TCMT.RowIndex, 0, sizeof(int) * (TSize + 1));

    for (int i = 0; i < TNonzeroSize; i++) {
        TCMT.RowIndex[CMT.Columns[i] + 1]++;
    }

    int offset = 0;
    int tmp;
    for (int i = 1; i <= TSize; i++) {
        tmp = TCMT.RowIndex[i];
        TCMT.RowIndex[i] = offset;
        offset += tmp;
    }

    for (int i = 0; i < TSize; i++) {
        int start = CMT.RowIndex[i];
        int end = CMT.RowIndex[i + 1];
        int Column = i;
        double Value;
        int CMTcolumn, TCMTindex;

        for (int j = start; j < end; j++) {
            Value = CMT.Values[j];
            CMTcolumn = CMT.Columns[j];
            TCMTindex = TCMT.RowIndex[CMTcolumn + 1];
            TCMT.Values[TCMTindex] = Value;
            TCMT.Columns[TCMTindex] = Column;
            TCMT.RowIndex[CMTcolumn + 1]++;
        }
    }

    return TCMT;
}

CRSMatrix seq_MultiplicationMatrCRS(CRSMatrix CMT_1,
    CRSMatrix TCMT_2) {
    CRSMatrix CMT_Rez;
    std::vector<double> Value;
    std::vector<int> Column, Row;

    int Size = CMT_1.Size;

    int NzSize = 0;

    Row.push_back(0);

    for (int i = 0; i < Size; i++) {
        for (int j = 0; j < Size; j++) {
            double ScalarMult = 0;

            int ks = CMT_1.RowIndex[i];
            int ls = TCMT_2.RowIndex[j];
            int kf = CMT_1.RowIndex[i + 1] - 1;
            int lf = TCMT_2.RowIndex[j + 1] - 1;

            while ((ks <= kf) && (ls <= lf)) {
                if (CMT_1.Columns[ks] < TCMT_2.Columns[ls]) {
                    ks++;
                } else {
                    if (CMT_1.Columns[ks] > TCMT_2.Columns[ls]) {
                        ls++;
                    } else {
                        ScalarMult += CMT_1.Values[ks] *
                            TCMT_2.Values[ls];
                        ks++;
                        ls++;
                    }
                }
            }

            if (ScalarMult != 0) {
                Column.push_back(j);
                Value.push_back(ScalarMult);
                NzSize++;
            }
        }
        Row.push_back(NzSize);
    }

    CreateCRSMatrix(Size, Value.size(), &CMT_Rez);

    for (size_t j = 0; j < Value.size(); j++) {
        CMT_Rez.Values[j] = Value[j];
        CMT_Rez.Columns[j] = Column[j];
    }

    for (int j = 0; j < Size + 1; j++)
        CMT_Rez.RowIndex[j] = Row[j];

    return CMT_Rez;
}

CRSMatrix pp_MultiplicationMatrCRS(CRSMatrix CMT_1,
    CRSMatrix TCMT_2, int num_of_threads) {
    int Size = CMT_1.Size;

    CRSMatrix CMT_Rez;

    int i;

    tbb::task_scheduler_init init(num_of_threads);

    std::vector<double>* Value = new std::vector<double>[Size];
    std::vector<int>* Column = new std::vector<int>[Size];
    int* Row = new int[Size + 1];

    memset(Row, 0, Size * sizeof(int));

    int chunk = 10;

    parallel_for(tbb::blocked_range<int>(0, CMT_1.Size, chunk),
    Multiplicator(CMT_1, TCMT_2, Value, Column, Row));

    int NZSize = 0;

    for (i = 0; i < Size; i++) {
        int tmp = Row[i];
        Row[i] = NZSize;
        NZSize += tmp;
    }
    Row[Size] = NZSize;

    CreateCRSMatrix(Size, NZSize, &CMT_Rez);

    int offset = 0;
    for (int j = 0; j < Size; j++) {
        int ColSize = Column[j].size();
        memcpy(&CMT_Rez.Columns[offset], &Column[j][0],
        ColSize * sizeof(int));
        memcpy(&CMT_Rez.Values[offset], &Value[j][0],
        ColSize * sizeof(double));
        offset += ColSize;
    }

    memcpy(&CMT_Rez.RowIndex[0], &Row[0], (Size + 1) *
    sizeof(int));

    delete[] Column;
    delete[] Row;
    delete[] Value;

    return CMT_Rez;
}

void Show_arr(int* arr, int size_arr) {
    if (arr == NULL || size_arr < 1) {
        return;
    }

    for (int i = 0; i < size_arr; i++) {
        std::cout << arr[i] << " ";
    }

    std::cout << std::endl;
}

void Show_arr(double* arr, int size_arr) {
    if (arr == NULL || size_arr < 1)
        return;

    for (int i = 0; i < size_arr; i++)
        std::cout << arr[i] << " ";

    std::cout << std::endl;
}

void Show_matr(double** matr, int size) {
    for (int i = 0; i < size; i++)
        Show_arr(matr[i], size);
}

void ShowCRSMatr(CRSMatrix CMT) {
    std::cout << std::endl;
    Show_arr(CMT.Values, CMT.NonzeroSize);
    Show_arr(CMT.RowIndex, CMT.Size + 1);
    Show_arr(CMT.Columns, CMT.NonzeroSize);
    std::cout << std::endl;
}

bool DenseMatrixsAreEqual(double** M_1, double** M_2, int Size) {
    for (int i = 0; i < Size; i++)
        for (int j = 0; j < Size; j++)
            if (M_1[i][j] != M_2[i][j])
                return false;

    return true;
}

double** Create_and_init_matr(int size) {
    if (size < 1)
        return NULL;

    double** matr;

    matr = new double*[size];
    for (int i = 0; i < size; i++)
        matr[i] = new double[size];

    srand((unsigned)time(NULL));

    if (size < 3) {
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                std::cout << "Enter element at the [" << i
                    << ", " << j << "] position:" << std::endl;
                std::cin >> matr[i][j];
                std::cout << std::endl;
            }
        }
    } else {
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                matr[i][j] = std::rand() % 100 - 50;
            }
        }
    }

    return matr;
}

double** seq_MultiplicationDenseMatrix(double** M_1,
    double** M_2, int Size) {
    double** M_Rez = Create_and_init_matr(Size);

    for (int i = 0; i < Size; i++)
        for (int j = 0; j < Size; j++) {
            M_Rez[i][j] = 0;
            for (int k = 0; k < Size; k++)
                M_Rez[i][j] += M_1[i][k] * M_2[k][j];
        }

    return M_Rez;
}

double** ConvertCRSToSimpleMatrix(CRSMatrix CMT) {
    double** MX = Create_and_init_matr(CMT.Size);
    int k = 0, l = 0;
    int SizeElOnRow = 0;

    for (int i = 0; i < CMT.Size; i++)
        for (int j = 0; j < CMT.Size; j++)
            MX[i][j] = 0;


    for (int i = 0; i < CMT.Size; i++) {
        SizeElOnRow = CMT.RowIndex[i + 1] - CMT.RowIndex[i];
        for (int j = 0; j < CMT.Size; j++)
            if (CMT.Columns[k] == j) {
                MX[i][j] = CMT.Values[k];
                k++;
                l++;
                if (l == SizeElOnRow)
                    break;
            }

        l = 0;
    }

    return MX;
}

bool CheckSeqAndSimpleRezults(CRSMatrix CMT_1,
    CRSMatrix CMT_2, CRSMatrix CMT_Rez) {
    if (CMT_1.Size > MAX_SIZE_DENSE_MATRIX) {
    std::cout << "Size of matrix must be less than "
        << MAX_SIZE_DENSE_MATRIX << std::endl;
        return false;
    }

    if (CMT_1.Size != CMT_2.Size) {
    std::cout << "Sizes of CMT_1 and CMT_2 differ " <<
        std::endl;
        return false;
    }

    double** CMT_1_simple = ConvertCRSToSimpleMatrix(CMT_1);
    double** CMT_2_simple = ConvertCRSToSimpleMatrix(CMT_2);
    double** CMT_Rez_simple = ConvertCRSToSimpleMatrix(CMT_Rez);
    double** Dense_Rez;

    Dense_Rez = seq_MultiplicationDenseMatrix(CMT_1_simple,
    CMT_2_simple, CMT_1.Size);

    if (CMT_1.Size < 10) {
        Show_matr(CMT_1_simple, CMT_1.Size);
    std::cout << std::endl;
        Show_matr(CMT_2_simple, CMT_1.Size);
    std::cout << std::endl;
        Show_matr(Dense_Rez, CMT_1.Size);
    std::cout << std::endl;
        Show_matr(CMT_Rez_simple, CMT_1.Size);
    }


    if (DenseMatrixsAreEqual(Dense_Rez, CMT_Rez_simple,
    CMT_1.Size)) {
    std::cout << "Multiplication of CRS Matrix is correct "
        << std::endl;
        delete[] CMT_1_simple;
        delete[] CMT_2_simple;
        delete[] CMT_Rez_simple;
        return true;
    } else {
    std::cout <<
        "Multiplication of CRS Matrix is not correct "
        << std::endl;
        delete[] CMT_1_simple;
        delete[] CMT_2_simple;
        delete[] CMT_Rez_simple;
        return false;
    }
}

bool CheckSeqAndPPRezults(CRSMatrix CMT_1, CRSMatrix CMT_2) {
    if ((CMT_1.Size != CMT_2.Size) || (CMT_1.NonzeroSize !=
    CMT_2.NonzeroSize))
        return false;

    for (int i = 0; i < CMT_1.NonzeroSize; i++)
        if ((CMT_1.Values[i] != CMT_2.Values[i]) ||
        (CMT_1.Columns[i] != CMT_2.Columns[i]))
            return false;

    for (int i = 0; i < CMT_1.Size + 1; i++)
        if (CMT_1.RowIndex[i] != CMT_2.RowIndex[i])
            return false;

    return true;
}


int main(int argc, char* argv[]) {
    int Size = 6;
    int NonzeroSize = 2;
    int num_of_threads = 2;

    CRSMatrix CMT_1, CMT_2, TCMT_2, seq_CMT_Rez, pp_CMT_Rez;

    double seq_start_time = 0.0;
    double seq_end_time = 0.0;
    double seq_work_time = 0.0;

    double pp_start_time = 0.0;
    double pp_end_time = 0.0;
    double pp_work_time = 0.0;

    std::cout << "Enter size of matrix:   ";
    std::cout << std::endl;

    std::cout <<
    "Enter the number of nonzero elements in the string:   ";
    std::cout << std::endl << std::endl;

    std::cout << "Enter the number of threads:   ";
    std::cout << std::endl << std::endl;

    if (NonzeroSize > Size || Size < 1 || NonzeroSize < 0) {
    std::cout << "Error" << std::endl;
    return 0;
    }

    InitCRSMatr(FIRST_CMT, Size, NonzeroSize, &CMT_1);
    InitCRSMatr(FIRST_CMT, Size, NonzeroSize, &CMT_2);

    TCMT_2 = Transposing(CMT_2);

    seq_start_time = clock();
    seq_CMT_Rez = seq_MultiplicationMatrCRS(CMT_1, TCMT_2);
    seq_end_time = clock();

    seq_work_time = (seq_end_time - seq_start_time)
    / CLOCKS_PER_SEC;

    if (Size <= 1000)
        CheckSeqAndSimpleRezults(CMT_1, CMT_2, seq_CMT_Rez);

    std::cout << std::endl <<
    "Sequence version of multiplication matrix is worked: "
    << seq_work_time << std::endl;


    pp_start_time = clock();
    pp_CMT_Rez = pp_MultiplicationMatrCRS(CMT_1, TCMT_2,
    num_of_threads);
    pp_end_time = clock();

    pp_work_time = (pp_end_time - pp_start_time)
    / CLOCKS_PER_SEC;

    std::cout << std::endl <<
    "Parallel version of multiplication matrix is worked: "
    << pp_work_time << std::endl;

    if (CheckSeqAndPPRezults(seq_CMT_Rez, pp_CMT_Rez))
    std::cout << std::endl <<
    "Rezults of PP and Seq versions are identical "
    << std::endl;
    else
    std::cout << std::endl <<
    " Rezults of PP and Seq versions are not identical "
    << std::endl;


    if (pp_work_time <= seq_work_time)
    std::cout << std::endl <<
    " Parallel version faster then sequence "
    <<std::endl;
    else
    std::cout << std::endl <<
    " Sequence version faster then parallel "
    << std::endl;

    DeleteCRSMatrix(&CMT_1);
    DeleteCRSMatrix(&CMT_2);
    DeleteCRSMatrix(&TCMT_2);
    DeleteCRSMatrix(&seq_CMT_Rez);

    return 0;
}
