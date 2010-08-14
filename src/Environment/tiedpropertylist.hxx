#ifndef __TIEDPROPERTYLIST_HXX
#define  __TIEDPROPERTYLIST_HXX
#include <simgear/props/props.hxx>
using simgear::PropertyList;

// Maybe this goes into SimGear's props.hxx later?
class TiedPropertyList : PropertyList {
public:
    TiedPropertyList() {}
    TiedPropertyList( SGPropertyNode_ptr root ) { _root = root; }

    void setRoot( SGPropertyNode_ptr root ) { _root = root; }
    SGPropertyNode_ptr getRoot() const { return _root; }

    template<typename T> SGPropertyNode_ptr Tie( SGPropertyNode_ptr node, const SGRawValue<T> &rawValue, bool useDefault = true  ) {
        bool success = node->tie( rawValue, useDefault );
        if( success ) {
            SG_LOG( SG_ALL, SG_INFO, "Tied " << node->getPath() );
            push_back( node );
        } else {
#if PROPS_STANDALONE
            cerr << "Failed to tie property " << node->getPath() << endl;
#else
            SG_LOG(SG_GENERAL, SG_WARN, "Failed to tie property " << node->getPath() );
#endif
        }
        return node;
    }

    template <class V> SGPropertyNode_ptr Tie( SGPropertyNode_ptr node, V * value, bool useDefault = true ) {
        return Tie( node, SGRawValuePointer<V>(value), useDefault );
    }

    template <class V> SGPropertyNode_ptr Tie( const char * relative_path, V * value, bool useDefault = true ) {
        return Tie( _root->getNode(relative_path,true), SGRawValuePointer<V>(value), useDefault );
    }

    template <class V> SGPropertyNode_ptr Tie( SGPropertyNode_ptr node, V (*getter)(), void (*setter)(V) = 0, bool useDefault = true ) {
        return Tie(node, SGRawValueFunctions<V>(getter, setter), useDefault );
    }

    template <class V> SGPropertyNode_ptr Tie( const char * relative_path, V (*getter)(), void (*setter)(V) = 0, bool useDefault = true ) {
        return Tie(_root->getNode(relative_path, true), SGRawValueFunctions<V>(getter, setter), useDefault );
    }

    template <class V> SGPropertyNode_ptr Tie( SGPropertyNode_ptr node, int index, V (*getter)(int), void (*setter)(int, V) = 0, bool useDefault = true) {
        return Tie( node, SGRawValueFunctionsIndexed<V>(index, getter, setter), useDefault );
    }

    template <class V> SGPropertyNode_ptr Tie( const char * relative_path, int index, V (*getter)(int), void (*setter)(int, V) = 0, bool useDefault = true) {
        return Tie( _root->getNode( relative_path, true ), SGRawValueFunctionsIndexed<V>(index, getter, setter), useDefault );
    }

    template <class T, class V> SGPropertyNode_ptr Tie( SGPropertyNode_ptr node, T * obj, V (T::*getter)() const, void (T::*setter)(V) = 0, bool useDefault = true) {
        return Tie( node, SGRawValueMethods<T,V>(*obj, getter, setter), useDefault );
    }

    template <class T, class V> SGPropertyNode_ptr Tie( const char * relative_path, T * obj, V (T::*getter)() const, void (T::*setter)(V) = 0, bool useDefault = true) {
        return Tie( _root->getNode( relative_path, true), SGRawValueMethods<T,V>(*obj, getter, setter), useDefault );
    }

    template <class T, class V> SGPropertyNode_ptr Tie( SGPropertyNode_ptr node, T * obj, int index, V (T::*getter)(int) const, void (T::*setter)(int, V) = 0, bool useDefault = true) {
        return Tie( node, SGRawValueMethodsIndexed<T,V>(*obj, index, getter, setter), useDefault);
    }

    template <class T, class V> SGPropertyNode_ptr Tie( const char * relative_path, T * obj, int index, V (T::*getter)(int) const, void (T::*setter)(int, V) = 0, bool useDefault = true) {
        return Tie( _root->getNode( relative_path, true ), SGRawValueMethodsIndexed<T,V>(*obj, index, getter, setter), useDefault);
    }

    void Untie() {
        while( size() > 0 ) {
            SG_LOG( SG_ALL, SG_INFO, "untie of " << back()->getPath() );
            back()->untie();
            pop_back();
        }
    }
private:
    SGPropertyNode_ptr _root;
};
#endif
