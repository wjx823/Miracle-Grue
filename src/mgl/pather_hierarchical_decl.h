/* 
 * File:   pather_hierarchical_decl.h
 * Author: Dev
 *
 * Created on December 14, 2012, 12:06 PM
 */

#ifndef MGL_PATHER_HIERARCHICAL_DECL_H
#define	MGL_PATHER_HIERARCHICAL_DECL_H

#include "ContainmentTree.h"
#include "spacial_data.h"
#include "pather_optimizer.h"
#include "spacial_graph.h"
#include "configuration.h"

/*
 Forward declare Json::Value so we don't need to include the header
 */
namespace Json {

class Value;

}

namespace mgl {

class HierarchyException : public Exception {
public:
    template <typename T>
    HierarchyException(const T& arg) : Exception(arg) {}
};

/**
 @brief A state tracking object for path optimization that primarily keeps 
 track of the most recently visited point.
 
 This is a subclass of Point2Type and most users can treat it as such.
 */
class OptimizerState : public Point2Type {
public:
    OptimizerState();
    OptimizerState(const Point2Type& other);
    OptimizerState(Scalar x, Scalar y);
    OptimizerState& operator=(const Point2Type& other);
    void setFirst(bool f);
    bool first() const;
private:
    bool m_isFirst;
};

class pather_hierarchical : public abstract_optimizer{
public:
    pather_hierarchical(const GrueConfig& grueConf);
    void addPath(const OpenPath& path, const PathLabel& label);
    void addPath(const Loop& loop, const PathLabel& label);
    void addBoundary(const OpenPath& path);
    void addBoundary(const Loop& loop);
    void clearPaths();
    void clearBoundaries();
protected:
    void optimizeInternal(LabeledOpenPaths& result);
private:
    /**
     @brief InsetTree describes the lowest type of node in the hierarchy. 
     
     A node in the inset tree consists of the loop and label describing 
     that particular inset, along with a spacial graph that contains 
     any non-inset objects that fall inside this inset, but not inside 
     any of its children. 
     
     Insideness considered geometrically only. For normal external insets, 
     the innermost insets will be geometrically inside the the outermost ones. 
     For insets of holes, the innermost insets will be geometrically 
     outside of outermost ones. This must be considered when traversing 
     the inset tree.
     
     Currently, this is the lowest element in the hierarchy, but this will not 
     always be so. In the future, I plan to keep track of solid and exposed 
     regions of a layer. These will be loops that are neither extruded nor 
     boundaries (tentatively) that represent things like exposed roofs
     and allow them to be pathed contiguously. These hierarchies will 
     be eventually contained in each inset tree node.
     */
    class InsetTree : public basic_containment_tree<InsetTree> {
    public:
        /// a convenience typedef to initialize the CRTP parent
        typedef basic_containment_tree<InsetTree> parent_class;
        
        /// construct a root inset tree
        InsetTree();
        /// construct a valid node in the inset tree
        InsetTree(const Loop& loop, const PathLabel& label);
        /**
         @brief insert a labeled path into the graph contained at this node
         @param path the path to insert
         @param label the label of the path
         
         This will store the path with its label into the SpacialGraph 
         object at this inset tree node. SpacialGraph handles 
         the specifics of maintaining its graph and r-tree, so we simply 
         forward the data to it.
         */
        void insert(const OpenPath& path, const PathLabel& label);
        /**
         @brief insert a labeled loop into the graph contained at this node
         @param loop the path to insert
         @param label the label of the loop
         
         DO NOT USE FOR INSETS! For inserting insets, instead call
         insert(InsetTree(loop, label));
         
         This will store the loop with its label into the SpacialGraph 
         object at this inset tree node. SpacialGraph handles 
         the specifics of maintaining its graph and r-tree, so we simply 
         forward the data to it.
         */
        void insert(const Loop& loop, const PathLabel& label);
        /**
         @brief Insert an inset into the InsetTree.
         @param other the node to insert. Will be destroyed by the process.
         @return reference to the node that holds the data from @a other
         
         This function forwards to basic_containment_tree's implementation. 
         I don't know why I need to do this since we're already a subclass.
         */
        InsetTree& insert(InsetTree& other);
        /**
         @brief Swap the contents of this object with @a other
         @param other The object with which to swap data.
         
         This function will swap the labels, invoke SpacialGraph's optimized 
         swap implementation, then call basic_containment_tree's swap
         to transfer over all its data.
         
         No new memory is allocated from this call. Biggest cost is updating 
         the pointers inside SpacialGraph
         */
        void swap(InsetTree& other);
        /**
         @brief Traverse this tree and its children, prioritizing according 
         to @a LABEL_COMPARE and respecting @a BOUNDARY_TEST, and optimizing 
         all contained graphs accordingly.
         @param LABEL_COMPARE A predicate on PathLabel
         @param BOUNDARY_TEST A functor on Segment2Type. 
         BOUNDARY_TEST::operator ()(const Segment2Type&) const will return 
         true if the line represents a valid extrusion path (i.e. does not 
         cross any boundaries), and false otherwise
         @param result Here will be placed the result of the traversal
         @param entryPoint Indicates where best to start traversal. After 
         function returns, holds the position of the last traversed point. 
         @param grueCfg Configuration object
         @param labeler instance of LABEL_COMPARE that dictates how best to 
         order labels
         @param bounder instance of BOUNDARY_TEST that dictates where 
         we can make new connections
         
         This function is destructive! It will cause all children to be 
         erased, and all graphs to be emptied.
         */
        template <typename LABEL_COMPARE, typename BOUNDARY_TEST>
        void traverse(LabeledOpenPaths& result, OptimizerState& entryPoint, 
                const GrueConfig& grueCfg, 
                const LABEL_COMPARE& labeler = LABEL_COMPARE(), 
                const BOUNDARY_TEST& bounder = BOUNDARY_TEST());
        /**
         @brief Print a simple ascii-art representation of this tree to @a out
         @param out ostream where to print the representation
         */
        void repr(std::ostream& out) const;
    private:
        /**
         @brief Helper function for the repr above. This one is recursive
         @param out where to print the representaion
         @param level the level of recursion - indent deeper children more
         */
        void repr(std::ostream& out, size_t level) const;
        /**
         @brief Select the best choice from my children that respects
         priorities imposed by labeler and restrictions imposed by 
         bounder.
         @param LABEL_COMPARE
         @param BOUNDARY_TEST
         @param entryPoint point used for distance testing
         @param labeler instance of object to compare labels
         @param bounder instance of object to test for new connections
         */
        template <typename LABEL_COMPARE, typename BOUNDARY_TEST>
        parent_class::iterator selectBestChild(OptimizerState& entryPoint, 
                const LABEL_COMPARE& labeler, 
                const BOUNDARY_TEST& bounder);
        /**
         @brief Traverse over the data contained inside this node only!
         This is called within traverse. Traverse decides in what order 
         to do the children, and at which point to traverse the current node. 
         This function traverses the current node.
         
         All parameters play the same role as in InsetTree::traverse(...)
         
         @param LABEL_COMPARE
         @param BOUNDARY_TEST
         @param result
         @param entryPoint
         @param grueCfg
         @param labeler
         @param bounder
         */
        template <typename LABEL_COMPARE, typename BOUNDARY_TEST>
        void traverseInternal(LabeledOpenPaths& result, 
                OptimizerState& entryPoint, 
                const GrueConfig& grueCfg, 
                const LABEL_COMPARE& labeler, 
                const BOUNDARY_TEST& bounder);
        //loop is stored by containment tree
        PathLabel m_label;
        SpacialGraph m_graph;
    };
    /**
     @brief OutlineTree describes the highest type of node in the hierarchy.
     
     As InsetTree describes a hierarchy of insets, OutlineTree describes 
     a hierarchy of outlines. This allows us to both group sibling 
     outlines and their contents together, and minimize the number of 
     boundaries that need to be considered for the optimization of 
     any single item.
     
     Topologically, these are laid out exactly as InsetTree. Each node in the 
     OutlineTree contains a spacial data structure of boundaries, 
     an InsetTree, and a SpacialGraph of items to be optimized separately 
     from the insets.
     */
    class OutlineTree : public basic_containment_tree<OutlineTree> {
    public:
        /// a convenience typedef to initialize the CRTP parent
        typedef basic_containment_tree<OutlineTree> parent_class;
        
        /// construct a root outline tree
        OutlineTree();
        /// construct a valid node in the outline tree
        OutlineTree(const Loop& loop);
        /**
         @brief insert a labeled path into the graph contained at this node
         @param path the path to insert
         @param label the label of the path
         
         This will store the path with its label into the SpacialGraph 
         object at this tree node, or the correct InsetTree node depending on 
         the label. SpacialGraph handles the specifics of maintaining its 
         graph and r-tree, so we simply forward the data to it.
         */
        void insert(const OpenPath& path, const PathLabel& label);
        /**
         @brief insert a labeled loop into the graph contained at this node
         @param loop the path to insert
         @param label the label of the loop
         
         DO NOT USE FOR OUTLINES! For inserting outlines, instead call
         insert(OutlineTree(loop));
         
         This will store the loop with its label into the SpacialGraph 
         object at this tree node, or the correct InsetTree node depending on 
         the label. SpacialGraph handles the specifics of maintaining its 
         graph and r-tree, so we simply forward the data to it.
         If label.isInset() returns true, we will add an entry to the 
         InsetTree at this node instead of adding it to a graph.
         */
        void insert(const Loop& loop, const PathLabel& label);
        /**
         Why do I need to write this? Is CRTP broken?
         */
        OutlineTree& insert(OutlineTree& other);
        /**
         @brief Swap the contents of this object with @a other
         @param other The object with which to swap data.
         
         This function will swap the InsetTrees, invoke SpacialGraph's optimized 
         swap implementation, then call basic_containment_tree's swap
         to transfer over all its data.
         
         No new memory is allocated from this call. Biggest cost is updating 
         the pointers inside SpacialGraph
         */
        void swap(OutlineTree& other);
        /**
         @brief Optimize myself and my children.
         
         In the most optimal order, optimize myself and my children. 
         Select from children based on distance.
         
         @param LABEL_COMPARE the type of object used to prioritize based on 
         labels.
         @param result here will be placed outcome of optimization
         @param entryPoint Indicates from where to start optimizing. 
         @param grueCfg The config object, used to select different policies
         When function returns, holds the position of last object to be 
         optimized.
         @param labeler instance of label comparison object. Passed to 
         InsetTree and SpacialGraph.
         
         This function is DESTRUCTIVE! It will cause all my children to 
         be erased and all my data to be emptied.
         */
        template <typename LABEL_COMPARE>
        void traverse(LabeledOpenPaths& result, OptimizerState& entryPoint, 
                const GrueConfig& grueCfg, 
                const LABEL_COMPARE& labeler = LABEL_COMPARE());
        /**
         @brief Optimize myself and my children using the same bounder object. 
         
         This is the same function as above, but instead of taking 
         advantage of the hierarchical layout, we generate a comprehensive 
         bounder object elsewhere and propagate it.
         
         
         @param LABEL_COMPARE same as above
         @param BOUNDARY_TEST type of bounder object, such as 
         basic_boundary_test
         @param result same as above
         @param entryPoint same as above
         @param grueCfg same as above
         @param labeler same as above
         @param bounder an instance of BOUNDARY_TEST to use.
         */
        template <typename LABEL_COMPARE, typename BOUNDARY_TEST>
        void traverse(LabeledOpenPaths& result, OptimizerState& entryPoint, 
                const GrueConfig& grueCfg, 
                const LABEL_COMPARE& labeler = LABEL_COMPARE(), 
                const BOUNDARY_TEST& bounder = BOUNDARY_TEST());
        /**
         @brief Print a simple ascii art representation of this tree to @a out
         @param out the ostream where to print representation
         */
        void repr(std::ostream& out) const;
        /**
         @brief Output to json the loops representing myself and all my children
         @param out the json value where result will be written
         
         This function will recurse to children, using the natural recursive 
         properties of json to make this trivial.
         
         All values take the form OUTLINE_FORM : 
         {
            type : "OutlineNode",
            loop : { output of dump_loop goes here, optionally }
            children : [list of OUTLINE_FORM]
         }
         
         This function does not yet output inset or graph. It might at some point.
         */
        void repr(Json::Value& out) const;
    private:
        /**
         @brief Full recursive repr function, called from the repr above
         @param out the ostream where to print things
         @param level Depth of recursion, indent children more
         */
        void repr(std::ostream& out, size_t level) const;
        /**
         @brief based on entryPoint, select best child
         @param entryPoint nearest to this point select child
         @return nearest child or end() if none exist
         
         Simply select the closest child to @a entryPoint based on the 
         distance from it to the child's boundary loop
         */
        iterator selectBestChild(OptimizerState& entryPoint);
        /**
         @brief Construct a collection of boundaries based on my and my 
         children's outlines.
         @param SPACIAL_CONTAINER the type of spacial container to be used. 
         Valid examples are rtree, local_rtree, quadtree, boxlist of 
         Segment2Tye.
         @param boundaries a properly initialized instance of spacial 
         container of the type specified. Boundaries will be inserted 
         into this container.
         
         We don't build up a collection of boundaries until we start 
         optimizing the contents of this node in the OutlineTree. 
         Calling this function will build up a collection of boundaries 
         relevant to this node.
         It's unnecessary to consider all boundaries. Looking at this node
         and its children is sufficient.
         */
        template <typename SPACIAL_CONTAINER>
        void constructBoundaries(SPACIAL_CONTAINER& boundaries) const;
        /**
         @brief Construct a collection of boundares of this node and all 
         of its descendents
         @param SPACIAL_CONTAINER type of container to use
         @param boundaries here will be written results
         
         This function works similarly to the above, but instead of 
         considering just the current layer, it will do a full 
         recursive buildup of ALL the boundaries.
         */
        template <typename SPACIAL_CONTAINER>
        void constructBoundariesRecursive(SPACIAL_CONTAINER& boundaries) const;
        
        
        InsetTree m_insets;
        SpacialGraph m_graph;
        
    };
    
    
    OutlineTree m_root;
    OptimizerState m_historyPoint;
    const GrueConfig& grueCfg;
};

}

#endif	/* MGL_PATHER_HIERARCHICAL_DECL_H */
