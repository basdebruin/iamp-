/*  

    Instant Amp PD External
    Based on how-to-example
    using allpass filters

    Olli's Dirty Trick

    coefficients:
      no phase shift:
        0.6923878
        0.9360654322959
        0.9882295226860
        0.9987488452737
      90° phase shift:
        0.4021921162426
        0.8561710882420
        0.9722909545651
        0.9952884791278

    out(t) = a^2*(in(t) + out(t-2)) - in(t-2)

  */


/**
 * include the interface to Pd 
 */
#include "m_pd.h"


/**
 * define a new "class" 
 */
static t_class *instant_amp_class;


/**
 * this is the dataspace of our new object
 * the first element is the mandatory "t_object"
 */
typedef struct _instant_amp {
  t_object  x_obj;
  t_sample f;

  //t_inlet  *x_in;
  t_outlet *x_out;
} t_instant_amp;


/* wrap function used by perform function */
unsigned int wrap(int in, unsigned int max) {
    if (in >= 0) {
        return in % max;
    } else {
        return wrap(max + in, max);
    }
}


/*

  ALLPASS FUNCTION
  takes: 
    input sample
    coefficient for current allpass filter
    buffer index for which buffer to use

  returns one allpassed signal

*/

t_sample allpass(t_sample input, t_sample coeff, unsigned int buffer_index) {

  // SETUP
  // creating static array of buffers for each allpass filter
  // buffers are 3 samples
  static t_sample xbuffer[8][3] = {
    { 0.0, 0.0, 0.0 },
    { 0.0, 0.0, 0.0 },
    { 0.0, 0.0, 0.0 },
    { 0.0, 0.0, 0.0 },

    { 0.0, 0.0, 0.0 },
    { 0.0, 0.0, 0.0 },
    { 0.0, 0.0, 0.0 },
    { 0.0, 0.0, 0.0 }
  };
  // y buffer only needs 3 buffer values, index is the same as xbuffer
  static t_sample ybuffer[8][3] = {
    { 0.0, 0.0, 0.0 },
    { 0.0, 0.0, 0.0 },
    { 0.0, 0.0, 0.0 },
    { 0.0, 0.0, 0.0 },

    { 0.0, 0.0, 0.0 },
    { 0.0, 0.0, 0.0 },
    { 0.0, 0.0, 0.0 },
    { 0.0, 0.0, 0.0 }
  };
  // setup buffer index array
  static int buffer_indices[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

  // make n equ current buffer index for convenience
  int n = buffer_indices[buffer_index];
  // increment buffer
  buffer_indices[buffer_index] = ( n + 1 ) % 3;
  n = buffer_indices[buffer_index]
  
  // set buffers
  xbuffer[buffer_index][n] =  input;

  // calc the allpass
  const t_sample out = (
    ( coeff * coeff ) *
    ( xbuffer[buffer_index][n] + 
      ybuffer[buffer_index][wrap( n-2, 3 )] ) - 
    xbuffer[buffer_index][wrap( n-2, 3 )]
  );

  ybuffer[buffer_index][n] = out;

  return out;

}

/*  
    Delays by one sample, nice and easy
*/
t_sample delay_by_one(t_sample input) {

  // save last
  static t_sample last_sample = 0.0;
  // intermediary variable
  const t_sample temp = last_sample;
  // set last_sample for next time
  last_sample = input;
  // and return the temp
  return temp;

}

/*
  First allpass function, uses allpass() and doesn't flip phase
*/

t_sample allpass_1(t_sample input) {

  // run through all pass filters, storing signal in sig variable
  t_sample sig = allpass(input, 0.6923878000000, 0);
           sig = allpass(sig,   0.9360654322959, 1);
           sig = allpass(sig,   0.9882295226860, 2);
           sig = allpass(sig,   0.9987488452737, 3);
           sig = delay_by_one(sig);

  return sig;
  
}

/*
  Second allpass function, uses allpass() and flips phase 90°
*/

t_sample allpass_2(t_sample input) {
  
  // run through all pass filters, storing signal in sig variable
  t_sample sig = allpass(input, 0.4021921162426, 4);
           sig = allpass(sig,   0.4021921162426, 5);
           sig = allpass(sig,   0.9722909545651, 6);
           sig = allpass(sig,   0.9952884791278, 7);

  return sig;
  
}


/**
 * this is the core of the object
 * this perform-routine is called for each signal block
 * the name of this function is arbitrary and is registered to Pd in the 
 * instant_amp_dsp() function, each time the DSP is turned on
 *
 * the argument to this function is just a pointer within an array
 * we have to know for ourselves how many elements inthis array are
 * reserved for us (hint: we declare the number of used elements in the
 * instant_amp_dsp() at registration
 *
 * since all elements are of type "t_int" we have to cast them to whatever
 * we think is apropriate; "apropriate" is how we registered this function
 * in instant_amp_dsp()
 */
t_int *instant_amp_perform(t_int *w)
{
  /* the first element is a pointer to the dataspace of this object */
  t_instant_amp *x = (t_instant_amp *)(w[1]);
  /* here is a pointer to the t_sample array */
  t_sample     *in =      (t_sample *)(w[2]);
  /* here comes the signalblock that will hold the output signal */
  t_sample    *out =      (t_sample *)(w[3]);
  /* all signalblocks are of the same length */
  int          len =             (int)(w[4]);

  // loop over samplebuffer
  for (int s = 0; s < len; s++) {

    // Allpass 1:
    t_sample ap1 = allpass_1(in[s]);

    // Allpass 2:
    t_sample ap2 = allpass_2(in[s]);

    // AMPLITUDE!
    out[s] = 0.5*(ap1+ap2);

  }

  // --------

  /* return a pointer to the dataspace for the next dsp-object */
  return (w+5);
}


/**
 * register a special perform-routine at the dsp-engine
 * this function gets called whenever the DSP is turned ON
 * the name of this function is registered in instant_amp_setup()
 */
void instant_amp_dsp(t_instant_amp *x, t_signal **sp)
{
  /* add instant_amp_perform() to the DSP-tree;
   * the instant_amp_perform() will expect "4" arguments (packed into an
   * t_int-array), which are:
   * the objects data-space, 2 signal vectors (which happen to be
   * 1 input signals and 1 output signal) and the length of the
   * signal vectors (all vectors are of the same length)
   */
  dsp_add(instant_amp_perform, 4, x,
          sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

/**
 * this is the "destructor" of the class;
 * it allows us to free dynamically allocated ressources
 */
void instant_amp_free(t_instant_amp *x)
{
  /* free any ressources associated with the given inlet */
  //inlet_free(x->x_in);

  /* free any ressources associated with the given outlet */
  outlet_free(x->x_out);
}

/**
 * this is the "constructor" of the class
 * the argument is the initial mixing-factor
 */
void *instant_amp_new()
{
  t_instant_amp *x = (t_instant_amp *)pd_new(instant_amp_class);
  
  /* create a new signal-inlet */
  //x->x_in = inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);

  /* create a new signal-outlet */
  x->x_out = outlet_new(&x->x_obj, &s_signal);

  return (void *)x;
}


/**
 * define the function-space of the class
 * within a single-object external the name of this function is very special
 */
void iamp_tilde_setup(void) {
  instant_amp_class = class_new(gensym("iamp~"),
        (t_newmethod)instant_amp_new,
        (t_method)instant_amp_free,
	sizeof(t_instant_amp),
        CLASS_DEFAULT, 
        0);

  /* whenever the audio-engine is turned on, the "instant_amp_dsp()" 
   * function will get called
   */
  class_addmethod(instant_amp_class,
        (t_method)instant_amp_dsp, gensym("dsp"), 0);

  /* if no signal is connected to the first inlet, we can as well 
   * connect a number box to it and use it as "signal" */
  CLASS_MAINSIGNALIN(instant_amp_class, t_instant_amp, f); 
}
