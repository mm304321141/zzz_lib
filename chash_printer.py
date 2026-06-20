"""GDB pretty-printer for contiguous_hash (chash.h).

Usage:
    (gdb) source /path/to/chash_printer.py

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
# contiguous_hash pretty-printer
# ---------------------------------------------------------------------------
#
# Relevant structures from chash.h:
#
#   struct hash_t        { hash_value_type hash; ... };   // sentinel = ~0
#   struct index_t       { hash_t hash; offset_type next; offset_type prev; };
#   struct value_t       { aligned_storage value_pod; };   // wraps value_type
#   struct root_t        { size_type bucket_count, capacity, size, free_count;
#                          offset_type free_list;
#                          float setting_load_factor;
#                          offset_type *bucket;
#                          index_t   *index;
#                          value_t   *value; };
#
# Key invariants used here:
#   - root_.size       = high-water mark of used slot indices (NOT element count)
#   - element count    = root_.size - root_.free_count
#   - slot i is empty  iff  root_.index[i].hash.hash == ~hash_value_type(0)
#   - root_.value[i]   is a value_t wrapper; the real value_type lives in
#     value_pod (aligned_storage). We must take &value_pod and reinterpret.
#
class CHashPrinter(PrinterBase):
    """Print a contiguous_hash."""

    class CHashIterator(Iterator):
        def __init__(self, val, value_type, empty_hash):
            self._val = val
            self._type = value_type
            self._empty = empty_hash
            self._pos = 0
            self._count = 0
            # root_.size is the high-water slot mark, not the element count.
            self._size = int(val['root_']['size'])

        def __iter__(self):
            return self

        def __next__(self):
            # Skip empty slots (hash sentinel == ~0).
            while True:
                if self._pos >= self._size:
                    raise StopIteration
                # index_t.hash is a hash_t, hash_t.hash is hash_value_type.
                h = self._val['root_']['index'][self._pos]['hash']['hash']
                if h == self._empty:
                    self._pos += 1
                    continue
                break

            # root_.value[pos] is a value_t (aligned_storage wrapper); the
            # actual value_type lives at &value_pod. Reinterpret-cast it.
            value_pod = self._val['root_']['value'][self._pos]['value_pod']
            item = value_pod.address.cast(self._type.pointer()).dereference()

            result = ('[%d]' % self._count, item)
            self._pos += 1
            self._count += 1
            return result

    def __init__(self, typename, val):
        self._typename = typename
        self._val = val
        gdb_type = val.type.strip_typedefs()
        type_name = gdb_type.name or str(gdb_type)
        self._element_type = gdb.lookup_type('%s::value_type' % type_name)
        hash_type = gdb.lookup_type('%s::hash_value_type' % type_name)
        # Empty-slot sentinel matches hash_t::clear(): ~hash_value_type(0).
        hash_type_str = hash_type.name or str(hash_type)
        self._empty_hash = gdb.parse_and_eval(
            '~static_cast<%s>(0)' % hash_type_str)

    def to_string(self):
        root = self._val['root_']
        # Element count = high-water slot count minus freed slots.
        count = root['size'] - root['free_count']
        return '%s<%s> with %s items' % (
            self._typename, self._element_type.name, count)

    def children(self):
        return self.CHashIterator(self._val, self._element_type,
                                  self._empty_hash)

    def display_hint(self):
        return 'map'


# ---------------------------------------------------------------------------
# Registration
# ---------------------------------------------------------------------------
def _chash_lookup(val):
    try:
        name = str(val.type.strip_typedefs())
    except Exception:
        return None
    # Match both bare and namespace-qualified instantiations.
    bare = name.split('<', 1)[0].rsplit('::', 1)[-1]
    if bare == 'contiguous_hash':
        return CHashPrinter('chash', val)
    return None


gdb.pretty_printers.append(_chash_lookup)
