#!/usr/bin/env bash

#Prints out the dimensions of the matrix.
dims() {
    echo $1
}

#Reflects the elements of the matrix along the main diagonal.
transpose() {
    echo $1
}

#Returns a row with each element representing a mean of the corresponding column.
mean() {
    echo $1
}

#Returns a new matrix which is a result of adding two matrices element-wise.
#Returns an error if two matrices have different dimensions.
add() {
    echo $1
}

#Produces an MxP matrix as a product of MxN and NxP matrices.
multiply() {
    echo $1
}