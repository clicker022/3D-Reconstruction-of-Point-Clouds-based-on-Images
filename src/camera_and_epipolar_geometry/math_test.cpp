#include <iostream>
#include <iomanip>
#include <math/matrix_svd.h>
#include "math/matrix.h"
#include "math/vector.h"

int main(int argc, char *argv[])
{
    //构建一个维度为4x5的矩阵，数据类型为double的矩阵
    math::Matrix<double, 4, 5> A;

    //矩阵元素的设置和访问
    int id=0;
    for(int i=0; i< A.rows; i++){
        for(int j=0; j< A.cols; j++){
            A(i,j) = ++id;
            std::cout<<std::setw(3)<<A(i, j);
        }
        std::cout<<std::endl;
    }
    std::cout<<std::endl;

    //取矩阵的列元素
    math::Vector<double, 4> col4 = A.col(4); // 取第5列元素
    std::cout<<"col4: "<<col4<<std::endl;

    //取矩阵的行元素
    math::Vector<double, 5> row2 = A.row(2); // 取第3行元素
    std::cout<<"row2: "<<row2<<std::endl;

    // 向量的创建
    math::Vector<double, 5> v1;
    for(int i=0; i<v1.dim; i++){
        v1[i] = i;
    }

    std::cout<<"v1: ";
    for(int i=0; i<v1.dim; i++){
        std::cout<<v1[i]<<" ";
    }
    std::cout<<std::endl<<std::endl;

    //奇异值分解
    math::Matrix<double, 4, 5>U;
    math::Matrix<double, 5, 5> S, V;
    math::matrix_svd<double, 4, 5> (A,&U, &S, &V);
    std::cout<<"U: "<<U<<std::endl;
    std::cout<<"S: "<<S<<std::endl;
    std::cout<<"V: "<<V<<std::endl;

    return 0;
}
