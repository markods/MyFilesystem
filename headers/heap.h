// _____________________________________________________________________________________________________________________________________________
// HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...H
// _____________________________________________________________________________________________________________________________________________
// HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...H
// _____________________________________________________________________________________________________________________________________________
// HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...H

#pragma once
#include <unordered_map>
#include <iosfwd>
#include <iomanip>
#include "!global.h"


// min binary heap with incremental updates of its elements' positions
// the heap requires that type T has:
// +   a public default constructor
// +   a public destructor
// +   a public operator<                      (only less is required)
// +   a public operator=                      (for swapping elements in heap)
// +   a template specialization for std::hash (for hashing elements in unordered map)
// +   a public operator==                     (for finding the element in the heap)
// +   a public ostream operator<<             (for debugging purposes)
template<typename T> class Heap
{
private:
    static const siz64 initcap = (1ULL<<3);   // initial heap capacity
    T nullelem;   // dummy element, returned if the requested element doesn't exist

private:
    T* elem;     // array of heap elements
    siz64 cnt;   // element count
    siz64 cap;   // capacity of the array that holds the heap elements

    std::unordered_map<T, idx64> map;    // hash map used for finding element positions in binary heap (useful for updating elements in heap)

public:
    Heap();                      // construct the heap
    ~Heap();                     // destruct the heap
    void clear();                // clear the heap

    bool push(const T& elem);    // insert element into heap, return true if insertion successful
    bool pop();                  // remove min element from heap, return true if removal successful

    siz64 size() const;          // return the size of the heap
    idx64 find(const T& elem);   // find the index of the given element, if the element doesn't exist return null index

    T& top();                    // return the min element, if the heap is empty return null element
    T& at(idx64 idx);            // return the element with the specified index, if the index is out of bounds return null element

    bool update(idx64 idx, T* oldkey = nullptr);   // reorder element with given idx in heap (move it up towards or away from the root (min element) of the heap), return true if successful
                                                   // use the given old key to update the hash map entry too (without the old value it wouldn't be possible to find it in O(log n))
    void rebuild();              // rebuild the heap (update entire heap starting as if the elements were added one by one, and rebuild the hash map as well)


    friend std::ostream& operator<<(std::ostream& os, Heap& h);     // preorder print heap to given ostream
    std::ostream& print(std::ostream& os, bool drawframe = true);   // preorder print heap to given ostream, and choose if the pretty print frame should be printed


private:
    bool fitcap();   // resize the underlying element array so that it better fits its elements

    constexpr static idx64 parent(idx64 idx);   // return the index of the parent      of the given element
    constexpr static idx64 lchild(idx64 idx);   // return the index of the left  child of the given element
    constexpr static idx64 rchild(idx64 idx);   // return the index of the right child of the given element
};



// construct the heap
template<typename T> Heap<T>::Heap()
{
    elem = new T[initcap];
    cnt = 0;
    cap = initcap;
}

// destruct the heap
template<typename T> Heap<T>::~Heap()
{
    // clear the elements from the map
    map.clear();
    // delete the element array
    if( elem ) { delete[] elem; elem = nullptr; }
    // reset the element count to zero
    cnt = 0;
    // reset the element array capacity to zero
    cap = 0;
}

// clear the heap
template<typename T> void Heap<T>::clear()
{
    // clear the elements from the map
    map.clear();
    // reset the element count to zero
    cnt = 0;
    // resize the underlying element array to its default value (since the element count is zero)
    fitcap();
}



// insert element into heap, return true if insertion successful
template<typename T> bool Heap<T>::push(const T& _elem)
{
    // if the size of the underlying array can't be increased, and it is full, then the new element can't be placed
    if( !fitcap() && cnt == cap ) return false;

    // save the mapping of the new element to its new location (at the end of the array)
    map[_elem] = cnt;
    // insert the new element at the end of the array
    elem[cnt] = _elem;
    // increase element count
    // this step must be done before update is called, since update needs to know the exact element count
    cnt++;

    // update the newly added element
    update(cnt-1);
    return true;
}

// remove min element from heap, return true if removal successful
template<typename T> bool Heap<T>::pop()
{
    // if the array is empty, no elements can be popped
    if( cnt == 0 ) return false;

    // remove the first (min) element from the hash mapping
    map.erase(elem[0]);
    // replace the first (min) element with the last element in the array (does nothing if there is only one element left, and it is the one being popped)
    elem[0] = elem[cnt-1];
    // decrease element count (thereby removing the last element from the array, but it is safely located at the beginning of the array)
    // this step must be done before update is called, since update needs to know the exact element count
    cnt--;

    // update the new first element (since it is not the minimal element, move it down the binary heap (tree) if needed)
    update(0);
    // resize the underlying array to better fit its elements
    fitcap();
    return true;
}



// return the size of the heap
template<typename T> siz64 Heap<T>::size() const { return cnt; }
// find the index of the given element, if the element doesn't exist return invalid index
template<typename T> idx64 Heap<T>::find(const T& _elem)
{
    // get the iterator to the given element from the hash map
    auto el = map.find(_elem);
    // if the (key, value) tuple exists (if the iterator is not at the cend() location), return its second element -- value
    return (el != map.cend()) ? el->second : nullidx64;
}

// return the min element, if the heap is empty return null element
template<typename T> T& Heap<T>::top() { return (cnt > 0) ? elem[0] : nullelem; }
// return the element with the specified index, if the index is out of bounds return null element
template<typename T> T& Heap<T>::at(idx64 idx) { return (idx < cnt) ? elem[idx] : nullelem; }



// reorder element with given idx in heap (move it up towards or away from the root (min element) of the heap), return true if successful
// use the given old key to update the hash map entry too (without the old value it wouldn't be possible to find it in O(log n))
template<typename T> bool Heap<T>::update(idx64 idx, T* oldkey)
{
    // if the current element index is out of bounds, return false
    if( idx >= cnt ) return false;

    // heapify up if necessary (swap element with parent if child < parent)
    for( idx64 next = idx; ; )
    {
        // if the current element is not the root element and is smaller than its parent, remember its parent index for swapping with the current element
        if( idx > 0 && elem[idx] < elem[parent(idx)] ) next = parent(idx);

        // if the current element should be swapped with itself, then heapify up is finished and break
        if( next == idx ) break;

        // swap the current element with the next element
        std::swap(elem[idx], elem[next]);
        // update the new current element index in the hash map
        map[elem[idx]] = idx;

        // continue the process for the new next element
        idx = next;
    }

    // heapify down if necessary (swap element with smallest child if it is greater than the smallest child)
    for( idx64 next = idx; ; )
    {
        // if the left child of the current element exists and it is smaller than the current element, remember its index for swapping with the current element
        if( lchild(idx) < cnt && elem[lchild(idx)] < elem[next] ) next = lchild(idx);
        // if the right child of the current element exists and it is smaller than both the current element and the left child, rememeber its index for swapping with the current element
        if( rchild(idx) < cnt && elem[rchild(idx)] < elem[next] ) next = rchild(idx);

        // if the current element should be swapped with itself, then heapify down is finished and break
        if( next == idx ) break;

        // swap the current element with the next element
        std::swap(elem[idx], elem[next]);
        // update the new current element index in the hash map
        map[elem[idx]] = idx;

        // continue the process for the new next element
        idx = next;
    }

    // if the key of the element changed before the update (before this function was called), remove the old entry from the hash map
    if( oldkey ) map.erase(*oldkey);
    // finally update the remaining element index in the hash map
    map[elem[idx]] = idx;
    return true;
}

// rebuild the heap (update entire heap starting as if the elements were added one by one, and rebuild the hash map as well)
template<typename T> void Heap<T>::rebuild()
{
    // save the old count value
    siz64 oldcnt = cnt;
    //clear the hash map
    map.clear();
    // this is similar to inserting an element at the end of the heap and then updating its position in heap
    // this way, the elements shouldn't move much from their original positions, and the entire update is incremental
    // at the end of the loop the element count will have its old value, so no further action is needed
    for( cnt = 0; cnt < oldcnt; cnt++ )
        update(cnt);   // update the newly added element (at the end of the element array)
}



// preorder print heap to given ostream
template<typename T> std::ostream& operator<<(std::ostream& os, Heap<T>& h) { return h.print(os, false); }

// preorder print heap to given ostream, and choose if the pretty print frame should be printed
template<typename T> std::ostream& Heap<T>::print(std::ostream& os, bool drawframe)
{
    // backup format flags and set fill character to zero (used if the frame should be drawn)
    std::ios_base::fmtflags f(os.flags());
    char c = os.fill('=');

    // draw the top part of the surrounding frame if it is requested
    if( drawframe )
        os << "======Heap<" << std::setw(4) << cnt << "," << std::setw(4) << map.size() << "," << std::setw(4) << cap << ">========" << std::endl;

    // set the stream fill character to a space
    os.fill(' ');

    // if the heap is not empty, preorder print it
    if( cnt > 0 )
    {
        // std::vector used as a stack (for preorder traversal of the heap)
        std::vector<idx64> preorder = { cnt };
        for( idx64 curr = 0; curr < cnt; )
        {
            // print the current element index in the heap (according to the hash map)
            os << std::setw(4) << map[elem[curr]] << ":";
            // print the element depth in the preorder traversal
            for( uns64 depth = curr+1; depth != 1; (depth >>= 1) )
                os << " | ";
            os << " + " << elem[curr] << "\n";

            // if the current element has a right child, add it to the preorder stack
            if( rchild(curr) < cnt )
            {
                preorder.push_back(rchild(curr));
            }

            // continue preorder traversal from the left child
            curr = lchild(curr);
            // if the left child doesn't exist, then
            if( curr >= cnt )
            {
                // take one right index child from the preorder stack, and continue the traversal from there on
                curr = preorder.back();
                preorder.pop_back();
            }
        }
    }
    // if the heap is empty, only print that info
    else
    {
        os << "<empty>\n";
    }

    // draw the bottom part of the surrounding frame if it is requested
    if( drawframe )
        os << "----------------------------------" << std::endl;

    // restore format flags and fill character
    os.flags(f); os.fill(c);
    return os;
}



// resize the underlying element array so that it better fits its elements
template<typename T> bool Heap<T>::fitcap()
{
    siz64 newcap = cap;
    // if the current count is greater than 85% of the capacity, double the capacity
    if     ( cnt >= cap*85/100 ) newcap <<= 1;
    // if the current count is less than 35% of the capacity, halve the capacity
    else if( cnt <= cap*35/100 ) newcap >>= 1;
    // if the new capacity is less than the default initial capacity, then set the new capacity to be exactly the initial capacity
    if( newcap < initcap ) newcap = initcap;

    // if there is no need for resizing, return true (as if the resize happened)
    if( newcap == cap ) return true;

    // create a new temporary array with the new capacity
    // the newcap has to be cast to a 32bit integer (or 32bit unsigned), since that is the argument to the new function
    T* temp = new T[(uns32) newcap];
    // if the allocation was unsuccessful, return false
    if( !temp ) return false;

    // copy the new capacity to the original variable
    cap = newcap;
    // copy the elements from the original array into the temporary array
    for( idx64 i = 0; i < cnt; i++ )
        temp[i] = elem[i];

    // delete the current array, and swap the current array pointer with the temporary array pointer
    delete[] elem; elem = temp;

    return true;
}

template<typename T> constexpr idx64 Heap<T>::parent(idx64 idx) { return ((++idx)>>1) - 1; }   // return the index of the left  child of the given element
template<typename T> constexpr idx64 Heap<T>::lchild(idx64 idx) { return ((++idx)<<1) - 1; }   // return the index of the left  child of the given element
template<typename T> constexpr idx64 Heap<T>::rchild(idx64 idx) { return ((++idx)<<1) + 0; }   // return the index of the right child of the given element





