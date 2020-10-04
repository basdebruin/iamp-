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
inline unsigned int wrap(int in, unsigned int max) {
    if (in >= 0) {
        return in % max;
    } else {
        return wrap(max + in, max);
    }
}


// basic allpass filter building block
#define AP(a) ((a*a)*(xbuffer[n] + ybuffer[wrap(n-2, 3)]) - xbuffer[wrap(n-2, 3)])

/*

  The first allpass filter, doesn't change the phase

*/
t_sample allpass_1(t_sample input) {
  // SETUP
  // buffers for x and y 
  static t_sample xbuffer[3] = { 0.0, 0.0, 0.0 };
  static t_sample ybuffer[3] = { 0.0, 0.0, 0.0 };

  // index
  static unsigned int n = 0;

  // calc the allpass
  const t_sample out = AP(0.6923878) * AP(0.9360654322959) * AP(0.9882295226860) * AP(0.9987488452737);

  //increment buffer after calculating for 1sample delay

  // increment buffer
  n = (n + 1) % 3;
  
  // set buffers
  xbuffer[n] = input;
  ybuffer[n] = input + out;

  // return
  return out;
}

/*

  The second allpass filter, flips the phase 90°

*/
t_sample allpass_2(t_sample input) {
  // SETUP
  // buffers for x and y
  static t_sample xbuffer[3] = { 0.0, 0.0, 0.0 };
  static t_sample ybuffer[3] = { 0.0, 0.0, 0.0 };

  // index
  static unsigned int n = 0;

  // increment buffer
  n = (n + 1) % 3;
  
  // set buffers
  xbuffer[n] = input;
  ybuffer[n] += input;

  // calc the allpass
  const t_sample out = AP(0.4021921162426) * AP(0.8561710882420) * AP(0.9722909545651) * AP(0.9952884791278);

  ybuffer[n] = out;

  // return
  return out;
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
