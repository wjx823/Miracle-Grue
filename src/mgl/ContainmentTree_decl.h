/* 
 * File:   ContainmentTree.h
 * Author: FILIPP
 *
 * Created on October 29, 2012, 12:16 PM
 */

#ifndef MGL_CONTAINMENTTREE_DECL_H
#define	MGL_CONTAINMENTTREE_DECL_H

#include "loop_path.h"
#include <list>

namespace mgl {


/**
 @brief A tree that represents objects containing other objects
 @param T an instance of this class is stored at each node in the tree
 
 A tree represents a single region (outline loop), and those 
 regions that fall inside this region. A tree contains its children 
 both in the OOP sense and in the spacial sense.
 
 This class stores at each level an object of type T
 that is contained by this object's boundary but NOT contained by the boundaries 
 of the children.
 
 The class @a T should support fast constant time swapping (by specializing 
 std::swap for itself) to keep insertion cost down.
 */
template <typename T>
class ContainmentTree {
public:
    /**
     @brief a convenience typedef for things we contain
     */
    typedef T value_type;
    /**
     @brief Construct a root tree
     */
    ContainmentTree();
    /**
     @brief Construct a normal tree
     @param loop a valid non-empty loop
     */
    ContainmentTree(const Loop& loop);
    /**
     @brief Test if this tree contains another hierarchy
     @param other does this contain other?
     @return whether this contains other
     
     Roots return true for all normal trees, false for other roots
     Normal trees use winding test for other normal trees, false for roots
     */
    bool contains(const ContainmentTree& other) const;
    /**
     @brief Test if this tree contains a this point
     @param point does this contain point?
     @return whether this contains @a point
     
     Normal trees use the winding test, invalid trees return true always.
     */
    bool contains(const Point2Type& point) const;
    /**
     @brief Test if this is a valid tree?
     
     Normal trees have a loop and other stuff. Invalid trees are roots
     @return true for normal tree, false for roots
     */
    bool isValid() const;
    /**
     @brief Return reference to deepest child that contains point, 
     or to this tree if there are no such children.
     @param point the point to test for containment
     @return reference to deepest tree that contains @a point
     */
    ContainmentTree& select(const Point2Type& point);
    /**
     @brief Return reference to deepest child that contains point, 
     or to this tree if there are no such children.
     @param point the point to test for containment
     @return reference to deepest tree that contains @a point
     */
    const ContainmentTree& select(const Point2Type& point) const;
    /**
     @brief Insert tree @a other into this tree
     @param other the tree to be inserted. This variable WILL be 
     invalidated after insertion. The returned reference will point 
     to the new tree which holds the contents of @a other
     @return reference to the tree with the contents of @a other
     
     Insert @a other into this tree. @a other will be invalidated by 
     this process, but a reference to a tree with the contents of 
     @a other is returned. This function will correctly handle cases 
     where @a other contains this or the children of this.
     
     Cost is proportional to the depth at which @a other is placed in 
     this tree times the branching factor of each tree traversed 
     while reaching that depth times the cost of each winding test.
     */
    ContainmentTree& insert(ContainmentTree& other);
    /**
     @brief Get a reference to the value stored by this tree
     @return Reference to the @a T stored by this object
     */
    value_type& value();
    /**
     @brief Get a reference to the value stored by this tree
     @return Const reference to the @a T stored by this object
     */
    const value_type& value() const;
    /**
     @brief Swap the contents of this tree with other.
     @param other The tree with which to swap contents
     
     Swap the contents of this tree with other. This involves swapping 
     each data member of this tree with the corresponding member in other. 
     Where possible, the fast, constant time implementation of swap is 
     used, so this function should run in constant time.
     */
    void swap(ContainmentTree& other);
private:
    typedef std::list<ContainmentTree> containment_list;
    
    Loop m_loop;
    containment_list m_children;
    value_type m_value;
};

}

namespace std {

/**
 @brief This is the ContainmentTree specialization of std::swap()
 */
template <typename T>
void swap(mgl::ContainmentTree<T>& lhs, mgl::ContainmentTree<T>& rhs);

}

#endif	/* MGL_CONTAINMENTTREE_DECL_H */
