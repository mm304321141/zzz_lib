"""GDB pretty-printer for segment_array (segment_array.h).

Usage:
    (gdb) source /path/to/segment_array_printer.py

Supports live debugging and core dumps. Compatible with Python 2/3.

The user-facing `segment_array<value_t, allocator_t>` is a `using`-alias for
`segment_array_implement<segment_array_config<...>>`, so the GDB type name
matches `segment_array_implement`.
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
# Relevant structures from segment_array.h (key-free B+ tree, same shape as
# bpptree.h with the per-inner key array removed):
#
#   struct node_t        { node_t *parent; size_t level; };
#   struct child_slot_t  { size_t size; node_t *ptr; };
#   struct inner_node_t : node_t
#     { child_slot_t children[max+1]; };
#     // Empty slots are encoded by children[i].ptr == nullptr (sentinel).
#   struct leaf_node_t  : node_t
#     { node_t *prev; node_t *next; value_type item[max]; };
#     // Leaf size is held in the parent slot's child_slot_t.size, except for
#     // the single-leaf-tree case where it lives in root_.size.
#   struct root_node_t  : node_t, ...
#     { size_t size; node_t *left; node_t *right; };
#     // root_.size      = total element count of the whole array
#     // root_.left      = leftmost leaf (or &root_ when empty)
#     // root_.parent    = root inner node (or self when empty)
#
# Iteration order (linear scan over leaves):
#   leaf = root_.left
#   while leaf != &root_:
#       sz = leaf_size(leaf)
#       for i in 0..sz-1: yield leaf->item[i]
#       leaf = leaf->next
# ---------------------------------------------------------------------------
class SegmentArrayPrinter(PrinterBase):
    """Print a segment_array."""

    class SegmentArrayIterator(Iterator):
        def __init__(self, val, leaf_type, inner_type, item_type, total):
            self._val = val
            self._leaf_type = leaf_type
            self._inner_type = inner_type
            self._item_type = item_type
            self._root_addr = int(val['root_'].address)
            self._total = int(total)
            self._index = 0
            self._leaf = None
            self._leaf_size = 0
            self._leaf_pos = 0
            self._prime()

        def __iter__(self):
            return self

        def _prime(self):
            if self._total == 0:
                return
            leaf_node = self._val['root_']['left']
            if int(leaf_node) == self._root_addr:
                return
            self._leaf = leaf_node.cast(self._leaf_type.pointer())
            self._leaf_size = self._leaf_size_of(self._leaf)
            self._leaf_pos = 0

        def _leaf_size_of(self, leaf_ptr):
            parent = leaf_ptr['parent']
            if int(parent) == self._root_addr:
                return int(self._val['root_']['size'])
            inner = parent.cast(self._inner_type.pointer())
            children = inner['children']
            leaf_addr = int(leaf_ptr)
            n = children.type.range()[1] + 1
            for i in range(n):
                slot = children[i]
                if int(slot['ptr']) == leaf_addr:
                    return int(slot['size'])
            return 0

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
        return self.SegmentArrayIterator(
            self._val, self._leaf_type, self._inner_type,
            self._value_type, total)

    def display_hint(self):
        return 'array'


def _segment_array_lookup(val):
    try:
        name = str(val.type.strip_typedefs())
    except Exception:
        return None
    bare = name.split('<', 1)[0].rsplit('::', 1)[-1]
    if bare == 'segment_array_implement':
        return SegmentArrayPrinter('segment_array', val)
    return None


gdb.pretty_printers.append(_segment_array_lookup)
