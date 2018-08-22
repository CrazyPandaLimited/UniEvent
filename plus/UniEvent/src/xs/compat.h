#pragma once

#if PERL_VERSION < 28
#undef gv_fetchmeth_sv
#define gv_fetchmeth_sv(a,b,c,d) Perl_gv_fetchmeth_pvn(aTHX_ a,SvPV_nolen(b),SvCUR(b),c,d)
#endif
