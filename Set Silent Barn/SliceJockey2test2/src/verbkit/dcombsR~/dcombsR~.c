/* 
[dcombsR~] class for Pure Data. Based on Jehar's freeverb. Written by 
Katja Vetter Jan. 2013.

The class has eight comb filters in parallel. Big float values or inf or NaN 
at the input are set to zero. A very small number, insignificant in terms of 
audio signals, is added to the input to avoid the computation of subnormals.
*/

#include "m_pd.h"
#include "bigorsmall.h"

#define STEREOSPREAD 23

#define DEL0 (1116 + STEREOSPREAD)
#define DEL1 (1188 + STEREOSPREAD)
#define DEL2 (1277 + STEREOSPREAD)
#define DEL3 (1356 + STEREOSPREAD)
#define DEL4 (1422 + STEREOSPREAD)
#define DEL5 (1491 + STEREOSPREAD)
#define DEL6 (1557 + STEREOSPREAD)
#define DEL7 (1617 + STEREOSPREAD)
#define NCOMBS 8

static const t_float fixedgain = 0.015;
static const t_float scaledecay = 0.28;
static const t_float offsetdecay = 0.7;
static const t_float initialdecay = 0.5;
static const t_float initialdamp = 0.3;
static const t_float scaledamp = 0.9;
static const t_float one = 1.;


t_class *dcombsR_class;


typedef struct
{
    t_float b0[DEL0];
    t_float b1[DEL1];
    t_float b2[DEL2];
    t_float b3[DEL3];
    t_float b4[DEL4];
    t_float b5[DEL5];
    t_float b6[DEL6];
    t_float b7[DEL7];
} t_bufs;


typedef struct
{
    t_object obj;
    t_float f;
    t_bufs bufs;
    t_float filter[8];
    t_float in;
    t_float damp;
    t_float attenuator;
    t_float feedback;
    int index[8];
} t_dcombsR;


#ifndef NOSIMD  // this code profits from SIMD instructions
static inline void dcombsR_process(t_dcombsR *x, t_float *input, t_float *output, int n)
{
    t_float damp = x->damp;
    t_float attenuator = x->attenuator;
    t_float feedback = x->feedback;
    t_float in, out;
    
    while(n--)
    {
        in = *input++ * fixedgain;
        if(bigfloat(in)) in = 0.;
        in += SMALLFLOAT;
        
        x->in = in;
        
        out = x->filter[0] + x->filter[1] + x->filter[2] + x->filter[3]
              + x->filter[4] + x->filter[5] + x->filter[6] + x->filter[7];
        
        out = flushsubliminal(out);
        *output++ = out;
        
        // combfilter sections
        x->filter[0] = (x->bufs.b0[x->index[0]] * attenuator) + (x->filter[0] * damp); 
        x->bufs.b0[x->index[0]++] = x->in + (x->filter[0] * feedback);
        if(x->index[0] == DEL0) x->index[0] = 0;
        
        x->filter[1] = (x->bufs.b1[x->index[1]] * attenuator) + (x->filter[1] * damp); 
        x->bufs.b1[x->index[1]++] = x->in + (x->filter[1] * feedback);
        if(x->index[1] == DEL1) x->index[1] = 0;
        
        x->filter[2] = (x->bufs.b2[x->index[2]] * attenuator) + (x->filter[2] * damp); 
        x->bufs.b2[x->index[2]++] = x->in + (x->filter[2] * feedback);
        if(x->index[2] == DEL2) x->index[2] = 0;
        
        x->filter[3] = (x->bufs.b3[x->index[3]] * attenuator) + (x->filter[3] * damp); 
        x->bufs.b3[x->index[3]++] = x->in + (x->filter[3] * feedback);
        if(x->index[3] == DEL3) x->index[3] = 0;
        
        x->filter[4] = (x->bufs.b4[x->index[4]] * attenuator) + (x->filter[4] * damp); 
        x->bufs.b4[x->index[4]++] = x->in + (x->filter[4] * feedback);
        if(x->index[4] == DEL4) x->index[4] = 0;
        
        x->filter[5] = (x->bufs.b5[x->index[5]] * attenuator) + (x->filter[5] * damp); 
        x->bufs.b5[x->index[5]++] = x->in + (x->filter[5] * feedback);
        if(x->index[5] == DEL5) x->index[5] = 0;
        
        x->filter[6] = (x->bufs.b6[x->index[6]] * attenuator) + (x->filter[6] * damp); 
        x->bufs.b6[x->index[6]++] = x->in+ (x->filter[6] * feedback);
        if(x->index[6] == DEL6) x->index[6] = 0;
        
        x->filter[7] = (x->bufs.b7[x->index[7]] * attenuator) + (x->filter[7] * damp); 
        x->bufs.b7[x->index[7]++] = x->in + (x->filter[7] * feedback);
        if(x->index[7] == DEL7) x->index[7] = 0;
        
        //out = out0 + out1 + out2 + out3 + out4 + out5 + out6 + out7;
        
    }
}
#endif  // end ifndef NOSIMD


#ifdef NOSIMD   // code optimised for archs without SIMD instructions
static inline void dcombsR_process(t_dcombsR *x, t_float *input, t_float *output, int n)
{
    t_float damp = x->damp;
    t_float attenuator = x->attenuator;
    t_float feedback = x->feedback;
    t_float in, out;
    
    int index0 = x->index[0];
    int index1 = x->index[1];
    int index2 = x->index[2];
    int index3 = x->index[3];
    int index4 = x->index[4];
    int index5 = x->index[5];
    int index6 = x->index[6];
    int index7 = x->index[7];
    
    t_float filter0 = x->filter[0];
    t_float filter1 = x->filter[1];
    t_float filter2 = x->filter[2];
    t_float filter3 = x->filter[3];
    t_float filter4 = x->filter[4];
    t_float filter5 = x->filter[5];
    t_float filter6 = x->filter[6];
    t_float filter7 = x->filter[7];
    
    t_float *buf0 = (t_float*)&x->bufs.b0[0];
    t_float *buf1 = (t_float*)&x->bufs.b1[0];
    t_float *buf2 = (t_float*)&x->bufs.b2[0];
    t_float *buf3 = (t_float*)&x->bufs.b3[0];
    t_float *buf4 = (t_float*)&x->bufs.b4[0];
    t_float *buf5 = (t_float*)&x->bufs.b5[0];
    t_float *buf6 = (t_float*)&x->bufs.b6[0];
    t_float *buf7 = (t_float*)&x->bufs.b7[0];
    
    while(n--)
    {
        in = *input++ * fixedgain;
        if(bigfloat(in)) in = 0.;
        in += SMALLFLOAT;
        
        out = filter0 + filter1 + filter2 + filter3
              + filter4 + filter5 + filter6 + filter7;
        
        out = flushsubliminal(out);
        *output++ = out;
        
        // combfilter sections
        filter0 = (buf0[index0] * attenuator) + (filter0 * damp);
        buf0[index0] = in + (filter0 * feedback);
        if(++index0 == DEL0) index0 = 0;
        
        filter1 = (buf1[index1] * attenuator) + (filter1 * damp);
        buf1[index1] = in + (filter1 * feedback);
        if(++index1 == DEL1) index1 = 0;
        
        filter2 = (buf2[index2] * attenuator) + (filter2 * damp);
        buf2[index2] = in + (filter2 * feedback);
        if(++index2 == DEL2) index2 = 0;
        
        filter3 = (buf3[index3] * attenuator) + (filter3 * damp);
        buf3[index3] = in + (filter3 * feedback);
        if(++index3 == DEL3) index3 = 0;
        
        filter4 = (buf4[index4] * attenuator) + (filter4 * damp);
        buf4[index4] = in + (filter4 * feedback);
        if(++index4 == DEL4) index4 = 0;
        
        filter5 = (buf5[index5] * attenuator) + (filter5 * damp);
        buf5[index5] = in + (filter5 * feedback);
        if(++index5 == DEL5) index5 = 0;
        
        filter6 = (buf6[index6] * attenuator) + (filter6 * damp);
        buf6[index6] = in + (filter6 * feedback);
        if(++index6 == DEL6) index6 = 0;
        
        filter7 = (buf7[index7] * attenuator) + (filter7 * damp);
        buf7[index7] = in + (filter7 * feedback);
        if(++index7 == DEL7) index7 = 0;
    }
    
    x->index[0] = index0;
    x->index[1] = index1;
    x->index[2] = index2;
    x->index[3] = index3;
    x->index[4] = index4;
    x->index[5] = index5;
    x->index[6] = index6;
    x->index[7] = index7;
    
    x->filter[0] = filter0;
    x->filter[1] = filter1;
    x->filter[2] = filter2;
    x->filter[3] = filter3;
    x->filter[4] = filter4;
    x->filter[5] = filter5;
    x->filter[6] = filter6;
    x->filter[7] = filter7;
}
#endif  // end ifdef NOSIMD


static t_int *dcombsR_perform(t_int *w)
{
    t_dcombsR *x = (t_dcombsR*)w[1];
    t_sample *input = (t_sample*)w[2];
    t_sample *output = (t_sample*)w[3];
    t_int vecsize = (int)w[4];
    
    dcombsR_process(x, input, output, vecsize);
    
    return(w+5);
}


static void dcombsR_damp(t_dcombsR *x, t_floatarg tone)
{
    if(tone < 0.) tone = 0.;
    if(tone > 1.) tone = 1.;
    t_float damp = (one - tone) * scaledamp;
    x->damp = damp;
    x->attenuator = 1. - damp;
}


static void dcombsR_decay(t_dcombsR *x, t_floatarg decay)
{
    if(decay < 0.) decay = 0.;
    if(decay > 1.) decay = 1.;
    x->feedback = (decay * scaledecay) + offsetdecay;
}


static void dcombsR_dsp(t_dcombsR *x, t_signal **sp)
{
    dsp_add(dcombsR_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}


static void *dcombsR_new(void)
{
    t_dcombsR *x = (t_dcombsR*)pd_new(dcombsR_class);
    inlet_new(&x->obj, &x->obj.ob_pd, &s_float, gensym("decay"));
    inlet_new(&x->obj, &x->obj.ob_pd, &s_float, gensym("tone"));
    outlet_new(&x->obj, &s_signal);
    x->in = 0.;
    dcombsR_damp(x, initialdamp);
    dcombsR_decay(x, initialdecay);
    return x;
}


static void dcombsR_test(t_dcombsR *x)
{
    post("feedback is %f", x->feedback);
    post("damp is %f", x->damp);
    post("attenuator is %f", x->attenuator);
}


void dcombsR_tilde_setup(void)
{
    dcombsR_class = class_new(gensym("dcombsR~"), (t_newmethod)dcombsR_new, 
        NULL, sizeof(t_dcombsR), CLASS_DEFAULT, A_NULL);
    CLASS_MAINSIGNALIN(dcombsR_class, t_dcombsR, f);
    class_addmethod(dcombsR_class, (t_method)dcombsR_dsp, gensym("dsp"),
        (t_atomtype) 0);
    class_addmethod(dcombsR_class, (t_method)dcombsR_decay, gensym("decay"), 
        A_FLOAT, 0); 
    class_addmethod(dcombsR_class, (t_method)dcombsR_damp, gensym("tone"), 
        A_FLOAT, 0);
    class_addmethod(dcombsR_class, (t_method)dcombsR_test, gensym("test"), 0);
    post("[dcombsR~] version 1.0 by Katja Vetter, based on Jezar's freeverb");
}

    