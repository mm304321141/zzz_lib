"""GDB pretty-printer for contiguous_hash (chash.h).

Usage:
    (gdb) source /path/to/chash_printer.py

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

def __lldb_init_module(debugger, internal_dict):
    if _AIME_USING_LLDB:
        _aime_register_lldb_provider(
            debugger, r'^(.*::)?contiguous_hash<.*>$',
            CHashPrinter, 'chash')
