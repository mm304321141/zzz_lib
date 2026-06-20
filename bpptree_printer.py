"""GDB pretty-printer for b_plus_plus_tree (bpptree.h).

Usage:
    (gdb) source /path/to/bpptree_printer.py

Supports live debugging and core dumps. Compatible with Python 2/3.

Covers the type alias families exposed via bpptree.h / bpptree_set.h /
bpptree_map.h: `b_plus_plus_tree<config>` and the user-facing aliases
`bpptree_set`, `bpptree_multiset`, `bpptree_map`, `bpptree_multimap` (all of
which are `using`-aliases for `b_plus_plus_tree<...>`, so the underlying
GDB type name matches `b_plus_plus_tree`).
"""

import gdb
import sys

if sys.version_info[0] > 2:
    Iterator = object
else:
    class Iterator(object):
        def next(self):
            return self.__next__()

PrinterBase = gdb.ValuePrinter if hasattr(gdb, 'ValuePrinter') else object


# ---------------------------------------------------------------------------
# Relevant structures from bpptree.h:
#
#   struct node_t        { node_t *parent; size_t level; };
#   struct child_slot_t  { size_t size; node_t *ptr; };
#   struct inner_node_t : node_t
#     { child_slot_t children[max+1]; key_type item[max]; };
#     // Empty slots are encoded by children[i].ptr == nullptr (sentinel).
#     // No `used` field is stored.
#   struct leaf_node_t  : node_t
#     { node_t *prev; node_t *next; storage_type item[max]; };
#     // Leaf size is NOT stored on the leaf itself.
#     // It lives in the parent slot's child_slot_t.size, except for the
#     // single-leaf-tree case where it lives in root_.size.
#   struct root_node_t  : node_t, ...
#     { size_t size; node_t *left; node_t *right; };
#     // root_.size      = total element count of the whole tree
#     // root_.left      = leftmost leaf (or &root_ when the tree is empty)
#     // root_.parent    = root inner node (or self when the tree is empty)
#
# Iteration order (in-order over leaves):
#   leaf = root_.left
#   while leaf != &root_:
#       sz = leaf_size(leaf)
#       for i in 0..sz-1: yield leaf->item[i]
#       leaf = leaf->next
# ---------------------------------------------------------------------------
class BpptreePrinter(PrinterBase):
    """Print a b_plus_plus_tree."""

    class BpptreeIterator(Iterator):
        def __init__(self, val, leaf_type, inner_type, item_type, total):
            self._val = val
            self._leaf_type = leaf_type
            self._inner_type = inner_type
            self._item_type = item_type
            self._root_addr = int(val['root_'].address)
            self._total = int(total)
            self._index = 0       # global element index (output key)
            self._leaf = None     # current leaf_node_t* (gdb.Value)
            self._leaf_size = 0
            self._leaf_pos = 0
            self._prime()

        def __iter__(self):
            return self

        # Locate the first leaf and its size. May leave _leaf as None when
        # the tree is empty.
        def _prime(self):
            if self._total == 0:
                return
            leaf_node = self._val['root_']['left']
            if int(leaf_node) == self._root_addr:
                return
            self._leaf = leaf_node.cast(self._leaf_type.pointer())
            self._leaf_size = self._leaf_size_of(self._leaf)
            self._leaf_pos = 0

        # leaf size lives in the parent slot, except for single-leaf trees
        # where the leaf's parent is &root_ and the size lives in root_.size.
        def _leaf_size_of(self, leaf_ptr):
            parent = leaf_ptr['parent']
            if int(parent) == self._root_addr:
                return int(self._val['root_']['size'])
            inner = parent.cast(self._inner_type.pointer())
            children = inner['children']
            leaf_addr = int(leaf_ptr)
            # children has length max+1; scan until ptr matches.
            n = children.type.range()[1] + 1
            for i in range(n):
                slot = children[i]
                if int(slot['ptr']) == leaf_addr:
                    return int(slot['size'])
            return 0  # should not happen with a well-formed tree

        def _advance_leaf(self):
            nxt = self._leaf['next']
            if int(nxt) == self._root_addr:
                self._leaf = None
                return
            self._leaf = nxt.cast(self._leaf_type.pointer())
            self._leaf_size = self._leaf_size_of(self._leaf)
            self._leaf_pos = 0

        def __next__(self):
            while True:
                if self._leaf is None or self._index >= self._total:
                    raise StopIteration
                if self._leaf_pos >= self._leaf_size:
                    self._advance_leaf()
                    continue
                item = self._leaf['item'][self._leaf_pos]
                key = '[%d]' % self._index
                self._leaf_pos += 1
                self._index += 1
                return (key, item)

    def __init__(self, typename, val):
        self._typename = typename
        self._val = val
        gdb_type = val.type.strip_typedefs()
        type_name = gdb_type.name or str(gdb_type)
        self._tname = type_name
        try:
            self._value_type = gdb.lookup_type('%s::value_type' % type_name)
        except gdb.error:
            self._value_type = None
        self._leaf_type = gdb.lookup_type('%s::leaf_node_t' % type_name)
        self._inner_type = gdb.lookup_type('%s::inner_node_t' % type_name)

    def to_string(self):
        count = int(self._val['root_']['size'])
        vname = self._value_type.name if self._value_type else '?'
        return '%s<%s> with %d items' % (self._typename, vname, count)

    def children(self):
        total = int(self._val['root_']['size'])
        return self.BpptreeIterator(
            self._val, self._leaf_type, self._inner_type,
            self._value_type, total)

    def display_hint(self):
        return 'array'


def _bpptree_lookup(val):
    try:
        name = str(val.type.strip_typedefs())
    except Exception:
        return None
    bare = name.split('<', 1)[0].rsplit('::', 1)[-1]
    if bare == 'b_plus_plus_tree':
        return BpptreePrinter('bpptree', val)
    return None


gdb.pretty_printers.append(_bpptree_lookup)
