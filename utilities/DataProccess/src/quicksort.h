/*
Author: Collin Dietz
Date: 3/28/17
Purpose: Genric, templated, quicksort for a vector of elements
*/

#ifndef QUICKSORT_H
#define QUICKSORT_H
#include<vector>

using namespace std;

//Utility function to swap the location of two generic items in a vector
//takes in the vector and the to indecies to swap
//assumes both indicies are inbounds
template<class T>
void swap(vector<T>& v, int a, int b);

//function to partition the given vector over range [l,r]
//picks r are the partition value and comapres using the given fucntion
//compares so that eveything left of the posistion of the partion
//returns true when compar(l, r) is called
template<class T>
int partition(vector<T>& a, int l, int r, bool(*compar)(T, T));

//quick sort function to sort over a given range [l,r] using
//the provided function
template<class T>
void qsort(vector<T>& a, int l, int r, bool(*compar)(T, T));

//quicksort function to sort the whole vector using the given
//comparision function so that every element compared with one to
//the right of it returns true
template<class T>
void qsort(vector<T>& a, bool(*compar)(T, T));

#endif
#include"quicksort.cpp"
