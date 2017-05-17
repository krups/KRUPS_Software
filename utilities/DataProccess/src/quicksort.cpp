/*
Author: Collin Dietz
Date: 3/28/17
*/

#ifndef QUICKSORT_CPP
#define QUICKSORT_CPP

#include<vector>
#include"quicksort.h"

using namespace std;

//swap function
template<class T>
void swap(vector<T>& v, int a, int b)
{
	T temp = v.at(a);
	v[a]= v.at(b);
	v[b]= temp;
}

//partition function, chooses r as partition
template<class T>
int partition(vector<T>& a, int l, int r, bool(*compar)(T, T))
{
	//check stop condition to be safe
	if(l == r)
	{
		return r;
	}
	// chose right most element as pivot i.e. r
	//compare with l most element until it is false
	while(compar(a.at(l), a.at(r)))
	{
		l++; //scoot left one 
		//check to make sure we aren't done
		if(l == r) return r;
	}

	//false value found 
	//Special swap case
	//if r is right next to l
	if(r == l+1)
	{
		//swap directly
		swap(a, l, r);
		//pivot is now moved at a spot one less  so stop condition (l == r) is true
		return r-1;
	}
	else
	{
		int junk = r-1;
		//otherwise swap r and junk first
		swap(a, r, junk);
		//pivot is now in correct position in array
		//put l in the correct position
		swap(a, r, l);
		//value at l is now pivot + 1, and junk value is the new test value
		//adjust r to where pivot is
		r--;
		//now partition the rest of the array (from l to r)
		// and return the pos of the pivot it brings back
		return partition(a, l, r, compar);
	}
}

//quick sort a generic list so that compar(l , r) is true for elements in the range [l,r]
template<class T>
void qsort(vector<T>& a, int l, int r, bool(*compar)(T, T))
{
	//if r is 'left' of or equal to l, then we stop
	if(r - l<= 0)
	{
		return;
	}
	else
	{
		int i = partition(a, l, r, compar); //partition the vector
		//qsort the remaining areas on either side
		qsort(a, l, i-1, compar); 
		qsort(a, i+1, r, compar);
	}
}

//qsort the whole vector so that comapr(l, r) is true for all elements
template<class T>
void qsort(vector<T>& a, bool(*compar)(T, T))
{
	qsort(a, 0, a.size()-1, compar);
}

#endif
