import sys
import tensorflow as tf
import functions
import core
from tensorflow.contrib.keras import backend as K

# arguments
dtype = int(sys.argv[1])
saving_dir = sys.argv[2]
computeMode = sys.argv[3]
fn_type = sys.argv[4]
gs_type = sys.argv[5]
gs_arg = sys.argv[6:]

__import__('graph_structure.' + gs_type)
gs = sys.modules['graph_structure.' + gs_type]

__import__('functions.' + fn_type)
fn = sys.modules['functions.' + fn_type]

gs_method = getattr(gs, gs_type)
fn_method = getattr(fn, fn_type)

config = tf.ConfigProto(
    intra_op_parallelism_threads=0,
    inter_op_parallelism_threads=0
)
session = tf.Session(config=config)
# session = tf.Session()
K.set_session(session)

# Device Configuration
GPU_mode, Dev_list = core.dev_config(computeMode)

with tf.device(Dev_list[0]):  # Base device(cpu mode: cpu0, gpu mode: first gpu on the list)
    gs_ob = gs_method(dtype, *gs_arg, fn=fn_method)
    fn_ob = fn_method(dtype, gs_ob)

file_name = fn_type + '_' + gs_type + '.pb'
initialize_all_variables_op = tf.variables_initializer(tf.global_variables(), name='initializeAllVariables')
tf.train.write_graph(session.graph_def, saving_dir, file_name, as_text=False)
