"""GDB pretty-printer for segment_array (segment_array.h).

Usage:
    (gdb) source /path/to/segment_array_printer.py

Supports live debugging and core dumps. Compatible with Python 2/3.

The user-facing `segment_array<value_t, allocator_t>` is a `using`-alias for
`segment_array_implement<segment_array_config<...>>`, so the GDB type name
matches `segment_array_implement`.
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

def __lldb_init_module(debugger, internal_dict):
    if _AIME_USING_LLDB:
        _aime_register_lldb_provider(
            debugger, r'^(.*::)?segment_array_implement<.*>$',
            SegmentArrayPrinter, 'segment_array')
