"""GDB pretty-printer for sparse_array (sparse_array.h).

Usage:
    (gdb) source /path/to/sparse_array_printer.py

Supports both live debugging and core dump analysis. Compatible with
Python 2 and Python 3.
"""

try:
    import lldb as _aime_lldb
    _AIME_USING_LLDB = True
except ImportError:
    _AIME_USING_LLDB = False
    import gdb  # noqa: F401

if _AIME_USING_LLDB:
    # LLDB-only branch: build a fake `gdb` module so the body Printer code
    # (which uses gdb.* APIs) can run unchanged under LLDB. We deliberately
    # do NOT rely on `try: import gdb` for environment detection, because
    # once any printer file installs the fake module into sys.modules['gdb'],
    # subsequent printer files would observe a successful `import gdb` and
    # misdetect as a GDB session.
    import sys as _aime_sys

    _aime_target = None

    class _AimeType(object):
        def __init__(self, t):
            self._t = t

        @property
        def name(self):
            n = self._t.GetName()
            return n if n else None

        def __str__(self):
            return self.name or ''

        def strip_typedefs(self):
            return _AimeType(self._t.GetCanonicalType())

        def pointer(self):
            return _AimeType(self._t.GetPointerType())

        def range(self):
            elem = self._t.GetArrayElementType()
            n = self._t.GetByteSize() // elem.GetByteSize()
            return (0, n - 1)

    class _AimeValue(object):
        def __init__(self, sb):
            self._sb = sb
            if sb is not None and sb.IsValid():
                global _aime_target
                _aime_target = sb.GetTarget()

        def __getitem__(self, key):
            sb = self._sb
            if isinstance(key, str):
                return _AimeValue(sb.GetChildMemberWithName(key))
            t = sb.GetType()
            if t.IsArrayType():
                return _AimeValue(sb.GetChildAtIndex(int(key)))
            pointee = t.GetPointeeType()
            base = sb.GetValueAsUnsigned()
            addr = base + int(key) * pointee.GetByteSize()
            target = _aime_target
            new_sb = target.CreateValueFromAddress(
                "[%d]" % int(key),
                _aime_lldb.SBAddress(addr, target), pointee)
            return _AimeValue(new_sb)

        def cast(self, ttype):
            return _AimeValue(self._sb.Cast(ttype._t))

        def dereference(self):
            return _AimeValue(self._sb.Dereference())

        @property
        def address(self):
            return _AimeValue(self._sb.AddressOf())

        @property
        def type(self):
            return _AimeType(self._sb.GetType())

        def __int__(self):
            return self._sb.GetValueAsUnsigned()

        def __index__(self):
            return self.__int__()

        def __long__(self):
            return self.__int__()

        def __eq__(self, other):
            if isinstance(other, _AimeValue):
                return int(self) == int(other)
            try:
                return int(self) == int(other)
            except Exception:
                return NotImplemented

        def __ne__(self, other):
            r = self.__eq__(other)
            if r is NotImplemented:
                return r
            return not r

        def __hash__(self):
            return int(self)

        def __add__(self, n):
            sb = self._sb
            t = sb.GetType()
            if t.IsPointerType():
                pointee = t.GetPointeeType()
                base = sb.GetValueAsUnsigned()
            else:
                pointee = t.GetArrayElementType()
                base = sb.GetLoadAddress()
            addr = base + int(n) * pointee.GetByteSize()
            target = _aime_target
            new_sb = target.CreateValueFromAddress(
                "_p", _aime_lldb.SBAddress(addr, target), pointee)
            return _AimePointer(new_sb, pointee, addr, target)

        def __sub__(self, other):
            if isinstance(other, _AimeValue):
                return int(self) - int(other)
            return int(self) - int(other)

        def __str__(self):
            v = self._sb.GetValue()
            return v if v is not None else ''

    class _AimePointer(_AimeValue):
        # Pointer-like value produced by `ptr + i` or by casting a raw int.
        # Subscripting [int] indexes from this position; [str] does field
        # access on the pointee.
        def __init__(self, sb_pointee, pointee_type, addr, target):
            self._sb = sb_pointee
            self._pointee = pointee_type
            self._addr = addr
            self._target = target
            global _aime_target
            _aime_target = target

        def __getitem__(self, key):
            if isinstance(key, str):
                return _AimeValue(self._sb.GetChildMemberWithName(key))
            offset = int(key)
            addr = self._addr + offset * self._pointee.GetByteSize()
            new_sb = self._target.CreateValueFromAddress(
                "[%d]" % offset,
                _aime_lldb.SBAddress(addr, self._target), self._pointee)
            return _AimeValue(new_sb)

        def __int__(self):
            return self._addr

        def __add__(self, n):
            addr = self._addr + int(n) * self._pointee.GetByteSize()
            new_sb = self._target.CreateValueFromAddress(
                "_p", _aime_lldb.SBAddress(addr, self._target),
                self._pointee)
            return _AimePointer(new_sb, self._pointee, addr, self._target)

    class _AimeRawAddr(object):
        # Wraps a raw integer address; only meaningful after .cast(ptr_type).
        def __init__(self, addr):
            self._addr = int(addr)

        def cast(self, ttype):
            target = _aime_target
            if target is None:
                target = _aime_lldb.debugger.GetSelectedTarget()
            ptr_t = ttype._t
            pointee = ptr_t.GetPointeeType()
            new_sb = target.CreateValueFromAddress(
                "_p", _aime_lldb.SBAddress(self._addr, target), pointee)
            return _AimePointer(new_sb, pointee, self._addr, target)

        def __int__(self):
            return self._addr

    class _AimeFakeGdb(object):
        error = Exception
        pretty_printers = []

        class ValuePrinter(object):
            pass

        @staticmethod
        def lookup_type(name):
            target = _aime_target
            if target is None:
                target = _aime_lldb.debugger.GetSelectedTarget()
            types = target.FindTypes(name)
            if types.GetSize() == 0:
                raise Exception("type not found: %s" % name)
            return _AimeType(types.GetTypeAtIndex(0))

        @staticmethod
        def parse_and_eval(expr):
            target = _aime_target
            if target is None:
                target = _aime_lldb.debugger.GetSelectedTarget()
            return _AimeValue(target.EvaluateExpression(expr))

        @staticmethod
        def Value(x):
            return _AimeRawAddr(x)

    _aime_fake_gdb = _AimeFakeGdb()
    _aime_sys.modules['gdb'] = _aime_fake_gdb
    import gdb  # noqa: F401

    def _aime_register_lldb_provider(debugger, type_regex, klass, short):
        printer_klass = klass

        class _Provider(object):
            def __init__(self, valobj, internal_dict):
                self.valobj = valobj
                self._items = []
                self._build()

            def _build(self):
                self._items = []
                try:
                    wrapped = _AimeValue(self.valobj.GetNonSyntheticValue())
                    p = printer_klass(short, wrapped)
                    for k, v in p.children():
                        self._items.append((k, v))
                except Exception:
                    pass

            def num_children(self):
                return len(self._items)

            def has_children(self):
                return True

            def get_child_index(self, name):
                for i, (k, _) in enumerate(self._items):
                    if k == name:
                        return i
                return -1

            def get_child_at_index(self, idx):
                if idx < 0 or idx >= len(self._items):
                    return None
                name, val = self._items[idx]
                sb = val._sb if isinstance(val, _AimeValue) else val
                t = sb.GetType()
                addr = sb.GetLoadAddress()
                if addr != _aime_lldb.LLDB_INVALID_ADDRESS:
                    return self.valobj.CreateValueFromAddress(name, addr, t)
                return self.valobj.CreateValueFromData(
                    name, sb.GetData(), t)

            def update(self):
                self._build()
                return False

        def _summary_fn(valobj, internal_dict):
            try:
                wrapped = _AimeValue(valobj.GetNonSyntheticValue())
                p = printer_klass(short, wrapped)
                return p.to_string()
            except Exception as e:
                return '<aime-error: %s>' % e

        provider_name = "_AimeProvider_%s" % short
        summary_name = "_aime_summary_%s" % short
        g = globals()
        g[provider_name] = _Provider
        g[summary_name] = _summary_fn

        mod = __name__
        debugger.HandleCommand(
            'type synthetic add -l %s.%s -x "%s"'
            % (mod, provider_name, type_regex))
        debugger.HandleCommand(
            'type summary add -F %s.%s -e -x "%s"'
            % (mod, summary_name, type_regex))
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

def __lldb_init_module(debugger, internal_dict):
    if _AIME_USING_LLDB:
        _aime_register_lldb_provider(
            debugger, r'^(.*::)?sparse_array<.*>$',
            SparseArrayPrinter, 'sparse_array')
