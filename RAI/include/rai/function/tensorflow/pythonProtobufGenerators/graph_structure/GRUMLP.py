import BaseClasses as bc
import tensorflow as tf
import tensorflow.contrib.rnn as rnn
from tensorflow.contrib.layers import fully_connected


# multiple gated recurrent unit layers (https://arxiv.org/pdf/1406.1078v3.pdf)
# Implementation of GRU + MLP layers
class GRUMLP(bc.GraphStructure):
    def __init__(self, dtype, *param, fn):
        super(GRUMLP, self).__init__(dtype)
        nonlin_str = param[0]
        nonlin = getattr(tf.nn, nonlin_str)
        weight = float(param[1])

        check=0
        for i, val in enumerate(param[2:]):
            if val == '/':
                check = i

        rnnDim = [int(i) for i in param[2:check+2]]
        mlpDim = [int(i) for i in param[check+3:]]

        self.input = tf.placeholder(dtype, shape=[None, None, rnnDim[0]], name=fn.input_names[0])  # [batch, time, dim]

        length_ = tf.placeholder(dtype, name='length')  # [batch]
        length_ = tf.cast(length_, dtype=tf.int32)
        self.seq_length = tf.reshape(length_, [-1])

        # GRU
        cells = []
        recurrent_state_size = 0

        for size in rnnDim[1:]:
            cell = rnn.GRUCell(size, activation=nonlin, kernel_initializer=tf.contrib.layers.xavier_initializer())
            cells.append(cell)
            recurrent_state_size += cell.state_size

        cell = rnn.MultiRNNCell(cells, state_is_tuple=False)
        hiddenStateDim = tf.identity(tf.constant(value=[recurrent_state_size], dtype=tf.int32), name='h_dim')

        init_state = tf.placeholder(dtype=dtype, shape=[None, recurrent_state_size], name='h_init')

        # Full-length output for training
        gruOutput, final_state = tf.nn.dynamic_rnn(cell=cell, inputs=self.input, sequence_length=self.seq_length, dtype=dtype, initial_state=init_state)
        # FCN
        top = tf.reshape(gruOutput,shape=[-1, rnnDim[-1]], name='fcIn')

        layer_n = 0
        for dim in mlpDim[:-1]:
            with tf.name_scope('hidden_layer'+repr(layer_n)):
                top = fully_connected(activation_fn=nonlin, inputs=top, num_outputs=dim, weights_initializer=tf.contrib.layers.xavier_initializer(), trainable=True)
                layer_n += 1

        with tf.name_scope('output_layer'):
            wo = tf.Variable(tf.random_uniform(dtype=dtype, shape=[mlpDim[-2], mlpDim[-1]], minval=-float(weight), maxval=float(weight)))
            bo = tf.Variable(tf.random_uniform(dtype=dtype, shape=[mlpDim[-1]], minval=-float(weight), maxval=float(weight)))
            top = tf.matmul(top, wo) + bo

        self.output = tf.reshape(top, [-1, tf.shape(self.input)[1], mlpDim[-1]])
        hiddenState = tf.identity(final_state, name='h_state')

        self.l_param_list = tf.get_collection(tf.GraphKeys.TRAINABLE_VARIABLES)
        self.a_param_list = self.l_param_list
        self.net = None
