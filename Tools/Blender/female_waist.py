"""A cintura do corpo feminino (10/11): a mesma malha e o mesmo esqueleto do humano, só a cintura mais fina e curvada até o quadril.
Usado pelo Tools/Blender/MakeFemaleBody.py (o corpo) e pelo MakeBlockOutfit.py (as roupas femininas acompanham a mesma curva).
Só mexe na posição dos pontos: os ossos e as animações continuam os do humano."""
import math

WAIST_Z, WAIST_W = .725, .085   # metros: o meio da cintura e a largura da curva
NARROW = .14                    # quanto afina no meio (14%)
HIPS_Z, HIPS_W, WIDEN = .56, .05, .03


def waist_k(z):
    return 1 - NARROW * math.exp(-((z - WAIST_Z) / WAIST_W) ** 2) + WIDEN * math.exp(-((z - HIPS_Z) / HIPS_W) ** 2)


def shape_point(co, reach=.2):
    """Afina um ponto do tronco (perto do meio do corpo); braços e pernas ficam como estão."""
    if .5 < co.z < .95 and abs(co.x) < reach:
        k = waist_k(co.z)
        co.x *= k
        co.y *= 1 - (1 - k) * .45


def _self_check():
    assert abs(waist_k(1.05) - 1) < 1e-3 and waist_k(WAIST_Z) < .9 and waist_k(HIPS_Z) > 1


_self_check()
