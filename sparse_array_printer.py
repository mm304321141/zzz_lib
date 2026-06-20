"""GDB pretty-printer for sparse_array (sparse_array.h).

Usage:
    (gdb) source /path/to/sparse_array_printer.py

Supports both live debugging and core dump analysis. Compatible with
Python 2 and Python 3.
"""

import gdb
import sys

# ---------------------------------------------------------------------------
# Python 2/3 compatibility boilerplate
# ---------------------------------------------------------------------------
if sys.version_info[0] > 2:
    Iterator = object
    long = int
else:
    class Iterator(object):
        """Compatibility mixin: write __next__, get next() for free."""
        def next(self):
            return self.__next__()

# Use gdb.ValuePrinter if available.
if hasattr(gdb, 'ValuePrinter'):
    PrinterBase = gdb.ValuePrinter
else:
    PrinterBase = object


# ---------------------------------------------------------------------------
# sparse_array pretty-printer
# ---------------------------------------------------------------------------
#
# Relevant structures from sparse_array.h (default handle_t == void *):
#
#   struct sparse_range          { uint32_t index; uint16_t length;
#                                  uint16_t offset; handle_t handle; };
#   struct sparse_range_set_base { handle_t parent_handle, left_handle,
#                                  right_handle; uint32_t black:1, nil:1,
#                                  length:30; };
#   struct sparse_range_set      : sparse_range_set_base
#                                  { uint32_t end; sparse_range begin[1]; };
#   struct memory_block_data     { union { value_t data[atomic_length]; ... }; };
#   struct memory_block          { handle_t next_handle, prev_handle;
#                                  memory_block_data data[1]; };
#
# Key invariants used here:
#   - index_tree_.root_ is the red-black "nil" sentinel; its handle equals
#     invalid_handle (0). It stores:
#       * parent_handle = tree root   (0 when the tree is empty)
#       * left_handle   = most-left   (in-order first range_set)
#       * right_handle  = most-right  (in-order last  range_set)
#   - each real tree node is a sparse_range_set: a sorted array
#     begin[0 .. end-1] of sparse_range.
#   - a sparse_range covers [index, index + length); its data lives in a
#     memory_block at `handle`, starting at slot `offset`. Consecutive
#     memory_block_data slots are contiguous, so the first slot's data[]
#     array can be indexed as a flat value_t[length].
#   - the rb-tree nil handle is invalid_handle (0).
#
class SparseArrayPrinter(PrinterBase):
    """Print a sparse_array as {index: value, ...}."""

    class SparseArrayIterator(Iterator):
        def __init__(self, val, range_set_type, block_type):
            self._val = val
            self._range_set_type = range_set_type
            self._block_type = block_type
            # In-order walk starts at the most-left range_set.
            root = val['index_tree_']['root_']
            self._node = self._handle(root['left_handle'])
            self._ri = 0       # current range index inside the range_set
            self._vi = 0       # current value index inside the range

        def __iter__(self):
            return self

        @staticmethod
        def _handle(h):
            # handle_t (void *) -> integer address; 0 means nil.
            try:
                return long(h)
            except Exception:
                return long(h.cast(gdb.lookup_type('unsigned long')))

        def _set_ptr(self, node):
            return gdb.Value(node).cast(self._range_set_type.pointer())

        def _successor(self, node):
            # In-order successor over the red-black tree (nil == handle 0).
            sp = self._set_ptr(node)
            right = self._handle(sp['right_handle'])
            if right != 0:
                node = right
                sp = self._set_ptr(node)
                left = self._handle(sp['left_handle'])
                while left != 0:
                    node = left
                    sp = self._set_ptr(node)
                    left = self._handle(sp['left_handle'])
                return node
            parent = self._handle(sp['parent_handle'])
            while parent != 0 and \
                    self._handle(self._set_ptr(parent)['left_handle']) != node:
                node = parent
                parent = self._handle(self._set_ptr(node)['parent_handle'])
            return parent

        def __next__(self):
            while True:
                if self._node == 0:
                    raise StopIteration
                rset = self._set_ptr(self._node)
                end = int(rset['end'])
                if self._ri >= end:
                    # advance to in-order successor range_set
                    self._node = self._successor(self._node)
                    self._ri = 0
                    self._vi = 0
                    continue
                rng = rset['begin'] + self._ri
                length = int(rng['length'])
                if self._vi >= length:
                    self._ri += 1
                    self._vi = 0
                    continue
                index = int(rng['index']) + self._vi
                offset = int(rng['offset'])
                block = gdb.Value(self._handle(rng['handle'])).cast(
                    self._block_type.pointer())
                value = block['data'][offset]['data'][self._vi]
                self._vi += 1
                return ('[%d]' % index, value)

    def __init__(self, typename, val):
        self._typename = typename
        self._val = val
        gdb_type = val.type.strip_typedefs()
        type_name = gdb_type.name or str(gdb_type)
        self._range_set_type = gdb.lookup_type(
            '%s::sparse_range_set' % type_name)
        self._block_type = gdb.lookup_type('%s::memory_block' % type_name)

    def _size(self):
        root = self._val['index_tree_']['root_']
        try:
            right = long(root['right_handle'])
        except Exception:
            right = long(root['right_handle'].cast(
                gdb.lookup_type('unsigned long')))
        if right == 0:
            return 0
        rset = gdb.Value(right).cast(self._range_set_type.pointer())
        last = rset['begin'] + (int(rset['end']) - 1)
        return int(last['index']) + int(last['length'])

    def to_string(self):
        return '%s with size %d' % (self._typename, self._size())

    def children(self):
        return self.SparseArrayIterator(
            self._val, self._range_set_type, self._block_type)

    def display_hint(self):
        return 'map'


# ---------------------------------------------------------------------------
# Registration
# ---------------------------------------------------------------------------
def _sparse_array_lookup(val):
    try:
        name = str(val.type.strip_typedefs())
    except Exception:
        return None
    # Match both bare and namespace-qualified instantiations.
    bare = name.split('<', 1)[0].rsplit('::', 1)[-1]
    if bare == 'sparse_array':
        return SparseArrayPrinter('sparse_array', val)
    return None


gdb.pretty_printers.append(_sparse_array_lookup)
