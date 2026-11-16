"""Peças para montar efeitos Niagara pelo Monolith (o editor aberto, porta 9316). Usado por BuildPowerFx.py.
Cada emissor nasce de um modelo do motor (estouro para todo lado, estouro em cone, fonte) e é ajustado por entradas."""
import json
from mono import call

DIR = '/Game/Vortex/Fx/Poderes/'
TPL = '/Niagara/DefaultAssets/Templates/Emitters/'
MOD = {'ScaleSpriteSize': '/Niagara/Modules/Update/Size/ScaleSpriteSize', 'ScaleColor': '/Niagara/Modules/Update/Color/ScaleColor',
       'SpriteRotationRate': '/Niagara/Modules/Update/Orientation/SpriteRotationRate',
       'SpriteFacingAndAlignment': '/Niagara/Modules/Update/Renderers/Sprite/SpriteFacingAndAlignment',
       'GravityForce': '/Niagara/Modules/Update/Forces/GravityForce', 'Drag': '/Niagara/Modules/Update/Forces/Drag'}
FAILS = []


def ok(r, what):
    if isinstance(r, dict) and 'ERROR' in r:
        FAILS.append((what, str(r['ERROR'])[:200]))
        print('  FALHOU', what, str(r['ERROR'])[:200])
        return {}
    return r


def vec(v):
    return '(X=%g,Y=%g,Z=%g)' % tuple(v)


class Sys:
    def __init__(self, name):
        self.name = name
        self.path = DIR + name
        self.end = 0.2
        call('niagara', 'create_system', {'save_path': self.path})
        listed = call('niagara', 'list_emitters', {'asset_path': self.path})
        if 'emitters' not in listed:
            raise RuntimeError('sem sistema ' + name + ': ' + str(listed)[:200])
        for e in listed['emitters']:   # refazer do zero
            call('niagara', 'remove_emitter', {'asset_path': self.path, 'emitter': e['name']})
        if 'User.Color' not in str(call('niagara', 'get_user_parameters', {'asset_path': self.path})):
            self.n('add_user_parameter', name='Color', type='LinearColor')
        call('niagara', 'set_parameter_default', {'asset_path': self.path, 'parameter': 'User.Color', 'value': '{"r":1,"g":0.45,"b":0.1,"a":1}'})

    def n(self, action, **p):
        what = self.name + ' ' + action + ' ' + str(p.get('emitter', '')) + ' ' + str(p.get('input', p.get('property', p.get('name', ''))))
        return ok(call('niagara', action, {'asset_path': self.path, **p}), what)

    def mods(self, e):
        s = self.n('get_emitter_summary', emitter=e)
        return {m['name']: m['guid'] for st in s.get('modules', {}).values() for m in st}

    def need(self, e, module, usage='particle_update'):
        if module not in self.mods(e):
            self.n('add_module', emitter=e, usage=usage, module_script=MOD[module])

    def set(self, e, module, **inputs):
        for k, v in inputs.items():
            self.n('set_module_input_value', emitter=e, module_node=module, input=k.replace('_', ' '), value=v if isinstance(v, str) else str(v))

    def switch(self, e, module, name, value):
        self.n('set_static_switch_value', emitter=e, module_node=module, input=name, value=str(value))

    def curve(self, e, module, inp, keys, vec2=False):
        """A entrada vira uma curva pela vida da partícula: keys = [(t, v), ...]."""
        self.need(e, module)
        tree = self.n('get_dynamic_input_tree', emitter=e, module_node=module)
        di = next((i.get('dynamic_input') for i in tree.get('inputs', []) if i['name'] == inp and i.get('dynamic_input')), None)
        float_from_curve = '/Niagara/DynamicInputs/ValueFromCurve/FloatFromCurve'
        if vec2:
            if not di:
                guid = self.n('add_dynamic_input', emitter=e, module_node=module, input=inp, dynamic_input_script='/Niagara/DynamicInputs/TypeConversions/Vector2DFromFloat').get('node_guid')
                di = {'node_guid': guid, 'inputs': []}
            inner = next((i.get('dynamic_input') for i in di.get('inputs', []) if i['name'] == 'Value' and i.get('dynamic_input')), None)
            if not inner:
                inner = {'node_guid': self.n('add_dynamic_input', emitter=e, module_node=di['node_guid'], input='Value', dynamic_input_script=float_from_curve).get('node_guid')}
            di = inner
        elif not di:
            di = {'node_guid': self.n('add_dynamic_input', emitter=e, module_node=module, input=inp, dynamic_input_script=float_from_curve).get('node_guid')}
        self.n('configure_curve_keys', emitter=e, module_node=di['node_guid'], input='FloatCurve', keys=[{'time': t, 'value': v} for t, v in keys], interp='linear')

    def rand(self, e, module, inp, lo, hi):
        tree = self.n('get_dynamic_input_tree', emitter=e, module_node=module)
        di = next((i.get('dynamic_input') for i in tree.get('inputs', []) if i['name'] == inp and i.get('dynamic_input')), None)
        guid = di['node_guid'] if di else self.n('add_dynamic_input', emitter=e, module_node=module, input=inp,
                                                  dynamic_input_script='/Niagara/DynamicInputs/UniformRange/V2/RandomRangeFloat').get('node_guid')
        for k, v in (('Minimum', lo), ('Maximum', hi)):
            self.n('set_dynamic_input_value', emitter=e, dynamic_input_node=guid, input=k, value=str(v))

    def mat(self, tex, blend, glow):
        name = 'MI_Fx_%s_%s%s' % (tex, blend, ('_%g' % glow).replace('.', 'p'))
        r = call('material', 'create_material_instance', {'asset_path': DIR + 'Mat/' + name, 'parent_material': DIR + ('M_FxParticulaAd' if blend == 'Ad' else 'M_FxParticula'),
                                                          'texture_parameters': {'Tex': '/Game/Vortex/Combat/Sprites/T_Fx_' + tex}, 'scalar_parameters': {'Brilho': glow}})
        if 'ERROR' not in r:
            call('material', 'save_material', {'asset_path': DIR + 'Mat/' + name})
        return DIR + 'Mat/' + name

    def emit(self, name, tex, base='omni', blend='Ad', glow=3.0, count=1, delay=0.0, life=(0.4, 0.6), size=(40, 60), speed=(0, 0), radius=0.0, gravity=0.0, drag=None,
             grow=None, fade=((0, 1), (0.6, 1), (1, 0)), facing=None, rot=(0, 360), spin=0.0, color='user', offset=None, cone=None, rate=None, dur=None, shape=None):
        """base: omni (estouro para todo lado), cone (estouro num cone), fonte (jorro contínuo num cone; rate = por segundo, dur = segundos).
        grow/fade: curvas [(t, v)] do tamanho e do alfa pela vida. facing: 'chao', 'frente' ou None (para a câmera). color: 'user' ou (r, g, b)."""
        e = name
        self.n('add_emitter', emitter_asset=TPL + 'OmnidirectionalBurst', name=e)   # um modelo só: ele já traz as curvas de tamanho e de alfa
        self.n('set_emitter_property', emitter=e, property='bLocalSpace', value='true')
        init = 'InitializeParticle'
        self.switch(e, init, 'Lifetime Mode', 'NewEnumerator1')
        self.set(e, init, Lifetime_Min=life[0], Lifetime_Max=life[1])
        self.switch(e, init, 'Sprite Size Mode', 'NewEnumerator1')
        self.set(e, init, Uniform_Sprite_Size=(size[0] + size[1]) * .5, Uniform_Sprite_Size_Min=size[0], Uniform_Sprite_Size_Max=size[1])
        self.switch(e, init, 'Sprite Rotation Mode', 'NewEnumerator2')
        self.set(e, init, Sprite_Rotation_Angle_Min=rot[0], Sprite_Rotation_Angle_Max=rot[1])
        self.switch(e, init, 'Color Mode', 'NewEnumerator1')
        if color == 'user':
            self.n('set_module_input_binding', emitter=e, module_node=init, input='Color', binding='User.Color')
        else:
            self.set(e, init, Color='(R=%g,G=%g,B=%g,A=1)' % tuple(color))
        if offset:
            self.switch(e, init, 'UsePositionOffset', 'true')
            self.set(e, init, Position_Offset=vec(offset))
        if base == 'fonte':
            self.n('add_module', emitter=e, usage='emitter_update', module_script='/Niagara/Modules/Emitter/SpawnRate')
            self.set(e, 'SpawnBurst_Instantaneous', Spawn_Count=0)
            self.set(e, 'SpawnRate', SpawnRate=rate or 40)
            self.n('set_emitter_loop_profile', emitter=e, loop_behavior='Once', loop_duration=dur or 0.5, loop_delay=delay, loop_delay_enabled=delay > 0)
            self.end = max(self.end, delay + (dur or 0.5) + life[1])
        else:
            self.set(e, 'SpawnBurst_Instantaneous', Spawn_Count=count, Spawn_Time=delay)
            self.n('set_emitter_loop_profile', emitter=e, loop_behavior='Once', loop_duration=delay + 0.1)
            self.end = max(self.end, delay + life[1])
        m = self.mods(e)
        if shape:
            self.n('set_spawn_shape', emitter=e, shape=shape[0], params=shape[1])
        elif 'ShapeLocation' in m:
            self.set(e, 'ShapeLocation', Sphere_Radius=max(radius, 6))
        elif radius:
            self.n('set_spawn_shape', emitter=e, shape='Sphere', params={'radius': radius})
        if 'AddVelocity' in m:
            self.rand(e, 'AddVelocity', 'Velocity Speed', speed[0], speed[1])
            if base != 'omni':
                self.switch(e, 'AddVelocity', 'Velocity Mode', 'NewEnumerator2')   # num cone
            if base == 'omni':   # o estouro sai do centro do próprio emissor (o modelo põe a origem 7 cm abaixo)
                self.set(e, 'AddVelocity', Origin_Offset=vec(offset or (0, 0, 0)))
            if cone:
                self.set(e, 'AddVelocity', Cone_Axis=vec(cone[0]), Cone_Angle=cone[1])
        if 'GravityForce' in m:
            self.set(e, 'GravityForce', Gravity=vec((0, 0, gravity)))
        if drag is not None and 'Drag' in m:
            self.set(e, 'Drag', Drag=drag)
        if 'ScaleSpriteSizeBySpeed' in m:
            self.n('remove_module', emitter=e, module_node=m['ScaleSpriteSizeBySpeed'])
        self.curve(e, 'ScaleSpriteSize', 'Scale Factor', grow or ((0, 1), (1, 1)), vec2=True)
        self.curve(e, 'ScaleColor', 'Scale Alpha', fade)
        if spin:
            self.need(e, 'SpriteRotationRate')
            self.set(e, 'SpriteRotationRate', Rotation_Rate=spin)
        if facing:
            self.need(e, 'SpriteFacingAndAlignment')
            self.set(e, 'SpriteFacingAndAlignment', Sprite_Facing='(X=0,Y=0,Z=1)' if facing == 'chao' else '(X=1,Y=0,Z=0)')
            self.n('set_renderer_property', emitter=e, renderer_index=0, property='FacingMode', value='CustomFacingVector')
        self.n('set_renderer_material', emitter=e, renderer_index=0, material=self.mat(tex, blend, glow))
        return e

    def done(self):
        self.n('request_compile')
        d = self.n('get_system_diagnostics')
        if d.get('error_count'):
            FAILS.append((self.name, str(d.get('errors'))[:300]))
            print('  ERROS', d.get('errors'))
        self.n('save_system', only_if_dirty=False)
        print(self.name, 'ok, dura', round(self.end, 2))
