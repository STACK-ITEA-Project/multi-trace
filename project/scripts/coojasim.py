#!/usr/bin/env python3

import xml.etree.ElementTree as ET

import coojautils


class ConfigBase:
    _element = None

    def __init__(self, e):
        self._element = e

    def add(self, name):
        c = ET.Element(name)
        self._element.append(c)
        return c

    def get(self, name):
        c = self._element.find(name)
        return None if c is None else c

    def get_or_add(self, name):
        c = self._element.find(name)
        return self.add(name) if c is None else c

    def remove(self, name):
        c = self._element.find(name)
        if c is not None:
            self._element.remove(c)
            return True
        return False

    def get_str(self, name, default=None):
        c = self._element.find(name)
        return default if c is None else c.text.strip()

    def set_str(self, name, v, create=True):
        c = self.get_or_add(name) if create else self._element.find(name)
        if c is not None:
            c.text = str(v)
            return True
        return False

    def get_int(self, name, default=None):
        v = self.get_str(name, default)
        return None if v is None else int(v)

    def set_int(self, name, v, create=True):
        return self.set_str(name, v, create)

    def get_float(self, name, default=None):
        v = self.get_str(name, default)
        return None if v is None else float(v)

    def set_float(self, name, v, create=True):
        return self.set_str(name, v, create)


class RadioMedium(ConfigBase):

    def __init__(self, e):
        super().__init__(e)


class UDGMRadioMedium(RadioMedium):

    def __init__(self, e):
        super().__init__(e)

    @property
    def transmitting_range(self):
        return self.get_float('transmitting_range')

    @transmitting_range.setter
    def transmitting_range(self, v):
        self.set_float('transmitting_range', v)

    @property
    def interference_range(self):
        return self.get_float('interference_range')

    @interference_range.setter
    def interference_range(self, v):
        self.set_float('interference_range', v)

    @property
    def success_ratio_tx(self):
        return self.get_float('success_ratio_tx', default=1.0)

    @success_ratio_tx.setter
    def success_ratio_tx(self, v):
        self.set_float('success_ratio_tx', v)

    @property
    def success_ratio_rx(self):
        return self.get_float('success_ratio_rx', default=1.0)

    @success_ratio_rx.setter
    def success_ratio_rx(self, v):
        self.set_float('success_ratio_rx', v)


class MoteType(ConfigBase):

    def __init__(self, e):
        super().__init__(e)


class InterfaceConfig(ConfigBase):

    def __init__(self, e):
        super().__init__(e)

    def get_type(self):
        return self._element.text.strip()


class Position(ConfigBase):

    def __init__(self, e):
        super().__init__(e)

    @property
    def x(self):
        return self.get_float('x')

    @x.setter
    def x(self, v):
        self.set_float('x', v)

    @property
    def y(self):
        return self.get_float('y')

    @y.setter
    def y(self, v):
        self.set_float('y', v)

    @property
    def z(self):
        return self.get_float('z')

    @z.setter
    def z(self, v):
        self.set_float('z', v)


class Mote(ConfigBase):

    _mote_id = None
    position = None
    interface_config = None

    def __init__(self, e):
        super().__init__(e)
        self.interface_config = []
        for c in e.iter('interface_config'):
            self.interface_config.append(InterfaceConfig(c))
            t = c.text.strip() if c.text else ''
            if t == 'org.contikios.cooja.interfaces.Position':
                self.position = Position(c)
            elif t == 'org.contikios.cooja.contikimote.interfaces.ContikiMoteID':
                self._mote_id = c

    @property
    def mote_id(self):
        e = None if self._mote_id is None else self._mote_id.find('id')
        return None if e is None else int(e.text.strip())

    @mote_id.setter
    def mote_id(self, new_id):
        e = None if self._mote_id is None else self._mote_id.find('id')
        if e is not None:
            e.text = new_id

    def get_mote_type_identifier(self):
        i = self._element.find('motetype_identifier')
        return None if i is None else i.text.strip()


class Simulation(ConfigBase):
    radio_medium = None
    motes = None
    motetypes = None

    def __init__(self, e):
        super().__init__(e)
        r = e.find('radiomedium')
        if r is not None:
            if r.text and r.text.strip() == 'org.contikios.cooja.radiomediums.UDGM':
                self.radio_medium = UDGMRadioMedium(r)
            else:
                self.radio_medium = RadioMedium(r)
        self.motes = []
        for m in e.iter('mote'):
            self.motes.append(Mote(m))
        self.motetypes = []
        for m in e.iter('motetype'):
            self.motetypes.append(MoteType(m))

    def get_motes(self, motetype=None):
        if not motetype:
            return list(self.motes)
        return [m for m in self.motes if m.get_mote_type_identifier() == motetype]


class Plugin(ConfigBase):

    def __init__(self, e):
        super().__init__(e)


class Cooja:

    _tree = None
    root = None
    sim = None
    plugins = None

    def __init__(self, filename):
        with coojautils.LogReader(filename) as f:
            self._tree = ET.parse(f)
            self.root = self._tree.getroot()
            if self.root.tag != 'simconf':
                raise ET.ParseError('Not a Cooja simulation file')
            s = self.root.find('simulation')
            if s is None:
                raise ET.ParseError('Not a Cooja simulation file')
            self.sim = Simulation(s)
            self.plugins = []
            for p in self.root.iter('plugin'):
                self.plugins.append(Plugin(p))

    def save(self, filename):
        # XML needs to be written via binary file stream
        with coojautils.LogWriter(filename, mode='wb') as f:
            self._tree.write(f, encoding='utf-8', xml_declaration=True)
