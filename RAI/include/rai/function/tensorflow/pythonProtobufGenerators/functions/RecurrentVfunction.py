import BaseClasses as bc
import tensorflow as tf
import core


class RecurrentVfunction(bc.SpecializedFunction):
    input_names = ['state']
    output_names = ['value']

    def __init__(self, dtype, gs):
        super(RecurrentVfunction, self).__init__(dtype, gs)
        # variables
        value = tf.squeeze(gs.output, axis=2, name=self.output_names[0])
        state_dim = gs.input.shape[1]
        state = gs.input
        clip_param = tf.Variable(0.2 * tf.ones(dtype=dtype, shape=[1, 1]), name='clip_param')

        # new placeholders
        value_target = tf.placeholder(dtype, shape=[None, None], name='targetValue')
        value_pred = tf.placeholder(dtype, shape=[None, None], name='predictedValue')

        mask = tf.sequence_mask(gs.seq_length, name='mask')

        # Assign ops.
        param_assign_placeholder = tf.placeholder(dtype, shape=[1, 1], name='param_assign_placeholder')
        tf.assign(clip_param, param_assign_placeholder, name='clip_param_assign')

        # solvers
        with tf.name_scope('trainUsingTargetValue'):
            core.square_loss_opt(dtype, value_target, value, tf.train.AdamOptimizer)

        with tf.name_scope('trainUsingTargetValue_huber'):
            core.huber_loss_opt(dtype, value_target, value, tf.train.AdamOptimizer)

        with tf.name_scope('trainUsingTRValue'):
            # Clipping-based trust region loss (https://github.com/openai/baselines/blob/master/baselines/pposgd/pposgd_simple.py)
            vfloss1 = tf.square(value - value_target)
            # clip_rate = clip_param[0]
            #
            # vpredclipped = value_pred - tf.clip_by_value(value - value_pred, -clip_rate, clip_rate)
            # vfloss2 = tf.square(vpredclipped - value_target)
            #
            # TR_loss = .5 * tf.reduce_mean(tf.maximum(vfloss1, vfloss2), name='loss')
            TR_loss = .5 * tf.reduce_mean(vfloss1, name='loss')

            learning_rate = tf.reshape(tf.placeholder(dtype, shape=[1], name='learningRate'), shape=[])
            train = tf.train.AdamOptimizer(learning_rate=learning_rate).minimize(TR_loss, name='solver',
                                                                                 colocate_gradients_with_ops=True)
