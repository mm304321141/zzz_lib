"""GDB pretty-printer for size_balanced_tree (sbtree.h).

Usage:
    (gdb) source /path/to/sbtree_printer.py

Supports live debugging and core dumps. Compatible with Python 2/3.

Covers the type alias families exposed via sbtree.h / sbtree_set.h /
sbtree_map.h: `size_balanced_tree<config>` and the user-facing aliases
`sbtree_set`, `sbtree_multiset`, `sbtree_map`, `sbtree_multimap` (all of
which are `using`-aliases for `size_balanced_tree<...>`, so the underlying
GDB type name matches `size_balanced_tree`).
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
# Relevant structures from sbtree.h:
#
#   struct node_t        { node_t *parent; node_t *left; node_t *right;
#                          size_t size; };
#   struct value_node_t : node_t  { value_type value; };
#   struct root_node_t  : node_t, key_compare, node_allocator_t { ... };
#   struct head_t       : root_allocator_t { root_node_t *root; };
#   class size_balanced_tree { head_t head_; ... };
#
# Layout invariants used here:
#   - head_.root is the "nil" sentinel:
#       * head_.root->parent  = real tree root (or self when empty)
#       * head_.root->left    = leftmost node  (or self when empty)
#       * head_.root->right   = rightmost node (or self when empty)
#       * head_.root->size    = 0   (empty-marker for is_nil_)
#   - For any real node n:  n->size = subtree node count (>= 1)
#   - In-order traversal:
#       start at head_.root->left (leftmost)
#       repeatedly take bst-next until reaching the nil sentinel
# ---------------------------------------------------------------------------
class SbtreePrinter(PrinterBase):
    """Print a size_balanced_tree."""

    class SbtreeIterator(Iterator):
        def __init__(self, val, value_node_type, value_type, nil_addr):
            self._val = val
            self._value_node_type = value_node_type
            self._value_type = value_type
            self._nil_addr = nil_addr
            self._index = 0
            # Total element count = root subtree size.
            head_root = val['head_']['root']
            tree_root = head_root['parent']
            if int(tree_root) == nil_addr:
                self._total = 0
                self._node = None
            else:
                self._total = int(tree_root['size'])
                # Leftmost node lives at nil->left.
                self._node = head_root['left']

        def __iter__(self):
            return self

        @staticmethod
        def _is_nil(node_ptr, nil_addr):
            return int(node_ptr) == nil_addr

        # In-order successor, identical to size_balanced_tree::bst_move_<true>.
        def _next_node(self, node):
            nil_addr = self._nil_addr
            right = node['right']
            if not self._is_nil(right, nil_addr):
                node = right
                while True:
                    left = node['left']
                    if self._is_nil(left, nil_addr):
                        return node
                    node = left
            # Climb until we come from the left.
            while True:
                parent = node['parent']
                if self._is_nil(parent, nil_addr):
                    return parent  # nil sentinel = end()
                if int(parent['left']) == int(node):
                    return parent
                node = parent

        def __next__(self):
            if self._node is None or self._is_nil(self._node, self._nil_addr):
                raise StopIteration
            value_node = self._node.cast(self._value_node_type.pointer())
            item = value_node['value']
            key = '[%d]' % self._index
            self._index += 1
            self._node = self._next_node(self._node)
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
        # value_node_t is a protected nested type; it is still observable to
        # gdb because debug info exposes it regardless of access control.
        self._value_node_type = gdb.lookup_type(
            '%s::value_node_t' % type_name)

    def _count(self):
        head_root = self._val['head_']['root']
        tree_root = head_root['parent']
        nil_addr = int(head_root)
        if int(tree_root) == nil_addr:
            return 0
        return int(tree_root['size'])

    def to_string(self):
        count = self._count()
        vname = self._value_type.name if self._value_type else '?'
        return '%s<%s> with %d items' % (self._typename, vname, count)

    def children(self):
        nil_addr = int(self._val['head_']['root'])
        return self.SbtreeIterator(
            self._val, self._value_node_type, self._value_type, nil_addr)

    def display_hint(self):
        return 'array'


def _sbtree_lookup(val):
    try:
        name = str(val.type.strip_typedefs())
    except Exception:
        return None
    bare = name.split('<', 1)[0].rsplit('::', 1)[-1]
    if bare == 'size_balanced_tree':
        return SbtreePrinter('sbtree', val)
    return None


gdb.pretty_printers.append(_sbtree_lookup)
