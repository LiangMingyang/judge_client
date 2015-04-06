#!/usr/bin/python
# coding: utf8
import os, sys
import traceback

main_filename = {
    'c++'   : 'main.cpp',
    'c'     : 'main.c',
    'python': 'main.py',
    'java'  : 'Main.java',
    'pascal': 'main.pas',
}

compiler_command = {
    'c++'   : 'g++ main.cpp -o main',
    'c'     : 'gcc main.c   -o main',
    'python': 'chmod +r /main.py',
    'java'  : 'javac Main.java',
    'pascal': 'fpc main.pas',
}

run_command = {
    'c++'   : '/main',
    'c'     : '/main',
    'python': 'python /main.py',
    'java'  : 'java Main',
    'pascal': '/main',
}

TEST_MASTER = 8193
TEST_PLAYER = 8194


# 输出汇报结果，不论正确与否
# score 得分
# time_used 时间消耗
# mem_used 内存消耗
# global_comment 全局评测结果
# detail_info 细节评测结果
# 评测结果输出到__report__文件里
def report(score, time_used, mem_used, global_comment, detail_info):
    repf = file('__report__', 'w')
    repf.write('%s, %d, %d\n%s' % (score.__str__(), time_used, mem_used, global_comment))
    if detail_info is not None:
        repf.write('\n%s' % detail_info)
    repf.close()
    sys.exit(0)

# 输出错误状态
# report_err('Compiler Error', 'Compiler: ' + cp_res['run result'])
# report_err('Compiler Error', cp_msg)
def report_err(comment, detail_info = None):
    report(0, 0, 0, comment, detail_info)

# 获取待评测程序所使用的语言
def get_lang():
    langf = file('/submission/__lang__', 'r')
    lang = langf.read()
    langf.close()
    return lang

def trim_setting():
    # read setting lines
    setting_file = file('/data/__setting_code__', 'r')
    setting_lines = setting_file.readlines()
    setting_file.close()
    # get setting
    setting = {}
    for line in setting_lines:
        line = line.split('#', 1)[0] # 每行的第一个 '#' 后面的全部忽略
        if line.find('=') >= 0: # 每行若没有 '=' 则忽略
            k, v = line.split('=', 1)
            if v.find(',') >= 0: # ‘=’ 右边的内容有 ',' 则是一个序列
                setting[k.lower().strip()] = [vi.lower().strip() for vi in v.split(',')]
            else: # '=' 右边的内容没有 ',' 则是一个值
                setting[k.lower().strip()] = v.lower().strip()
    # trim setting to real_set and return (13 item in total)
    config_errs = []

    # 该函数用来校验数值型参数的合法性
    def check_integer_value(value, low, high, err_comment):
        try:
            ret = int(value) # 转化数值
            # 两种不合法情况：低于下界或高于上界
            if (low is not None and ret < low) or (high is not None and ret > high):
                raise Exception('Error', 'Value not in range.')
            return ret
        except:
            config_errs.append(err_comment)
            # 错误列表：
            # "testcase_count" must be a integer in [1, 100].
            # "memory_limit" must be a integer in [0, 204800].
            # "process_limit" must be a integer in [1, 100].
            # "time_limit_global" must be a integer in [0, 30000].
            # "time_limit_case" must be some integers in [0, 30000] split by ",".
            # "compare_pe_level" must be a integer in [0, compare_ac_level].
            # "compare_ac_level" must be a integer in [0, 3].
            # "compare_tab_width" must be a integer in [0, 100].
            # "compare_pe_case_insensitive" must be a integer in [0, 1].
            # "compare_special" must be a integer in [0, 1].


    # real_set是最终的评测机设置，包含一系列键值
    # test_round_count 为测试轮数
    # memory_limit 内存限制
    # process_limit 进程数限制
    # time_limit_global 全局时限
    # time_limit_case 每轮的时限
    #standard_input_files 标准输入文件列表
    #standart_output_files 标准输出文件列表
    # round_weight 每轮权重
    # comepare_ac_level AC评测等级
    # compare_pe_level PE评测等级
    # compare_tab_width \t的宽度
    # compare_pe_case_insensitive PE是否为大小写
    # compare_special 是否启用Special Judge
    

    
    # config_errs 用来存储错误信息
    real_set = {}
    real_set['test_round_count']  = check_integer_value(setting.get('test_round_count', 1),
        1, 100, '"testcase_count" must be a integer in [1, 100].')
    real_set['memory_limit']      = check_integer_value(setting.get('memory_limit', 65535),
        0, 204800, '"memory_limit" must be a integer in [0, 204800].')
    real_set['process_limit']     = check_integer_value(setting.get('process_limit', 5),
        1, 100, '"process_limit" must be a integer in [1, 100].')
    real_set['time_limit_global'] = check_integer_value(setting.get('time_limit_global', 1000),
        0, 30000, '"time_limit_global" must be a integer in [0, 30000].')
    real_set['time_limit_case']   = [real_set['time_limit_global'] for i in range(0, real_set['test_round_count'])]
    if setting.has_key('time_limit_case'):
        tlc_str_list = setting['time_limit_case'] if type(setting['time_limit_case']) == list else [setting['time_limit_case'], ]
        for i in range(0, real_set['test_round_count']):
            if i >= tlc_str_list.__len__(): break
            real_set['time_limit_case'][i] = check_integer_value(tlc_str_list[i], 0, 30000,
                '"time_limit_case" must be some integers in [0, 30000] split by ",".')
    real_set['standard_input_files'] = [('/data/%d.in' % i) for i in range(0, real_set['test_round_count'])]
    if setting.has_key('standard_input_files'):
        stdin_files = setting['standard_input_files'] if type(setting['standard_input_files']) == list else [setting['standard_input_files'], ]
        for i in range(0, real_set['test_round_count']):
            if i >= stdin_files.__len__(): break
            real_set['standard_input_files'][i] = '/data/' + stdin_files[i]
    real_set['standard_output_files'] = [('/data/%d.out' % i) for i in range(0, real_set['test_round_count'])]
    if setting.has_key('standard_output_files'):
        stdout_files = setting['standard_output_files'] if type(setting['standard_output_files']) == list else [setting['standard_output_files'], ]
        for i in range(0, real_set['test_round_count']):
            if i >= stdout_files.__len__(): break
            real_set['standard_output_files'][i] = '/data/' + stdout_files[i]
    real_set['round_weight'] = [1 for i in range(0, real_set['test_round_count'])]
    if setting.has_key('round_weight'):
        rw_str_list = setting['round_weight'] if type(setting['round_weight']) == list else [setting['round_weight'], ]
        for i in range(0, real_set['test_round_count']):
            if i >= rw_str_list.__len__(): break
            real_set['round_weight'][i] = check_integer_value(rw_str_list[i], 1, 10000,
                '"round_weight" must be some integers in [1, 10000] split by ",".')
    real_set['compare_ac_level'] = check_integer_value(setting.get('compare_ac_level', 2),
        0, 3, '"compare_ac_level" must be a integer in [0, 3].')
    real_set['compare_pe_level'] = check_integer_value(setting.get('compare_pe_level', 0),
        0, real_set['compare_ac_level'], '"compare_pe_level" must be a integer in [0, compare_ac_level].')
    real_set['compare_tab_width'] = check_integer_value(setting.get('compare_tab_width', 4),
        0, 100, '"compare_tab_width" must be a integer in [0, 100].')
    real_set['compare_pe_case_insensitive'] = check_integer_value(setting.get('compare_pe_case_insensitive', 1),
        0, 1, '"compare_pe_case_insensitive" must be a integer in [0, 1].')
    real_set['compare_special'] = check_integer_value(setting.get('compare_special', 0),
        0, 1, '"compare_special" must be a integer in [0, 1].')

    # 上面用来设置各种评测参数

    # 如果config_errs不为空，那么有错误，报告错误，这里的错误是评测机设置有误
    # 即 Test setting error
    if config_errs.__len__() > 0:
        report_err('Test setting error', '\n'.join(config_errs))
    return real_set


# 用于运行judge并监控、记录结果
# command是编译指令
# time_limit是时间限制，默认为100s（100000ms）
# mem_limit 内存限制，默认为65535kb，即64M
# process_limit 进程数限制，默认为5
# uid和gid目前不明觉厉，应该是从数据库中取出的东西
# cpu_mask不明
# stdin_file为标准输入文件，待评测代码所在
# stdout_file为标准输出文件，评测结果存储的对应位置
# stderr_file标准错误输出文件，若评测出现某些错误，则输出到这个文件
# rejected_syscall不明觉厉= =猜测是被拒绝的一些函数调用，防止恶意注入
def judge_process_monitor(command, time_limit = 100000, mem_limit = 65535, process_limit = 5,
    uid = TEST_MASTER, gid = TEST_MASTER, cpu_mask = None, stdin_file = "/dev/null", stdout_file = "/dev/null",
    stderr_file = "/dev/null", rejected_syscall = []):
    
    # options对应某个指令的参数列表
    options = ''
    if time_limit is not None:    options = options + ' -t %d' % time_limit # 时限不为空则设置
    if mem_limit is not None:     options = options + ' -m %d' % mem_limit # 空间设置不为空则设置
    if process_limit is not None: options = options + ' -p %d' % process_limit # 进程限制不为空则设置
    if uid is not None:           options = options + ' -uid %d' % uid # uid不为空则设置
    if gid is not None:           options = options + ' -gid %d' % gid# git不为空则设置
    if cpu_mask is not None:      options = options + ' -cm %d' % cpu_mask # cpu_mask不为空则设置
    if stdin_file is not None:    options = options + ' -inf %s' % stdin_file # 输入文件不为空则设置
    if stdout_file is not None:   options = options + ' -outf %s' % stdout_file # 输出文件不为空则设置
    if stderr_file is not None:   options = options + ' -errf %s' % stderr_file # 错误文件不为空则设置
    for rsc in rejected_syscall:  options = options + ' -rsc %d' % rsc # syscall不为空则设置
    # os.popen用来执行shell指令，返回文件并读取结果。
    # 也就是意味着下面这些是用来执行judge_process_monitor的指令并且将运行结果返回，存储在res_line里面的
    res_lines = os.popen('judge_process_monitor %s %s' % (options, command)).readlines()
    # res_lines仅仅存储一行行的文本，具体评测结果经过解析后，以key(k):value(v)的形式存储在res里
    res = {}
    for line in res_lines:
        if line.find(':') >= 0:
            k, v = line.split(':', 1)
            res[k.lower().strip()] = v.strip()
    return res

special_compare_path = '/data/__compare__'


#基于setting['compare_special']
#决定是否启用special judge
# 0 则不启用
# 否则启用
def prepare_special_compare():
    if setting['compare_special'] == 0: return None
    make_time_limit = 10000 # ms
    make_mem_limit  = 500 * 1024 # KB
    make_process_limit = 100
    make_msg_file   = '__make_msg__'
    make_com = 'make --directory data'
    make_res = judge_process_monitor(
        command = make_com,
        time_limit = make_time_limit,
        mem_limit = make_mem_limit,
        process_limit = make_process_limit,
        stdout_file = make_msg_file,
        stderr_file = make_msg_file,
    )
    if make_res['run result code'] == '0':
        if make_res['exit code'] != '0':
            make_msg_f = file(make_msg_file, 'r')
            make_msg = make_msg_f.read()
            make_msg_f.close()
            report_err('Make special judge error', make_msg)
        else:
            if not os.path.exists(special_compare_path):
                report_err('Make special judge error', 'Special judge program "%s" not found' % special_compare_path)
    else:
        report_err('Make special judge error', 'Make special judge: ' + make_res['run result'])


# 准备待评测代码
# lang为代码使用的语言
# setting为先前的评测设置
def prepare_srcfile(lang, setting):
	# 如果代码使用的语言不在支持范围内，那么不被接受，汇报错误
	# main_filename = {
	#     'c++'   : 'main.cpp',
	#     'c'     : 'main.c',
	#     'python': 'main.py',
	#     'java'  : 'Main.java',
	#     'pascal': 'main.pas',
	# }
    if lang not in main_filename.keys():
        report_err('Submission error', 'Language "%s" is not supported' % lang)


    # 否则调用SHELL指令：
    # cp -p /submission/__main__ +对应语言的文件名
    # -p选项意味着完全复制文件，包括修改时间和访问权限等
    # 将待评测文件复制到评测目录里
    os.system('cp -p /submission/__main__ %s' % main_filename[lang])


# 准备编译器
def compiler(lang, setting):
    cp_time_limit = 10000 # ms 最长十秒 对应TLE
    cp_mem_limit  = 200 * 1024 # KB 最大.....没看懂多大 对应MLE
    cp_process_limit = 100 # 最多进程数 对应PLE
    cp_msg_file   = '__cp_msg__' #输出信息对应存储的文件名
    cp_com = compiler_command[lang] # 确定编译指令
 	#compiler_command = {
	#     'c++'   : 'g++ main.cpp -o main',  C语言指令
	#     'c'     : 'gcc main.c   -o main',  c++指令
	#     'python': 'chmod +r /main.py',     python直接赋予读（+r为read）权利
	#     'java'  : 'javac Main.java',       调用java编译器
	#     'pascal': 'fpc main.pas',          这个....用于pascal编译，具体不清楚
	#}

    if cp_com:
    	# 设置评测机相关的各项参数
        cp_res = judge_process_monitor(
            command = cp_com, # 编译指令，见上边注释
            time_limit = cp_time_limit, #时间限制
            mem_limit = cp_mem_limit,   #内存限制
            process_limit = cp_process_limit, #进程限制
            stdout_file = cp_msg_file, #标准输出文件
            stderr_file = cp_msg_file, #标准错误输出文件
        )
        if cp_res['run result code'] == '0':
        	# 正常编译
            if cp_res['exit code'] != '0':
            	# exit(0)的情况下，属于普通编译错误
            	# 读取错误信息，错误信息已经在上面的judge_process_monitor函数中完成写入
                cp_msg_f = file(cp_msg_file, 'r')
                cp_msg = cp_msg_f.read()
                cp_msg_f.close()
                report_err('Compiler Error', cp_msg)
        else:
        	# 下面的report error似乎是在评测没有正常进行时输出的编译错误
        	# 不过似乎从来没有出现过= =
            report_err('Compiler Error', 'Compiler: ' + cp_res['run result'])
        # 如果正常运行，那么评测结果的输出在judge_process_monitor中进行，不在这里。

def compare(stdin_file, stdout_file, program_stdout_file, setting):
    if setting['compare_special']:
        compare_command = '%s %s %s %s' % (special_compare_path, stdin_file, stdout_file, program_stdout_file)
    else:
        compare_command = 'judge_cmp %s %s %d %d %d %d' % (stdout_file, program_stdout_file,
            setting['compare_ac_level'], setting['compare_pe_level'],
            setting['compare_tab_width'], setting['compare_pe_case_insensitive'])
    compare_time_limit = 10000 # ms
    compare_mem_limit  = 200 * 1024 # KB
    compare_process_limit = 100
    compare_msg_file   = '__compare_msg__'
    os.system('rm -r -f %s' % compare_msg_file)
    compare_res = judge_process_monitor(
        command = compare_command,
        time_limit = compare_time_limit,
        mem_limit = compare_mem_limit,
        process_limit = compare_process_limit,
        stdout_file = compare_msg_file,
        stderr_file = compare_msg_file,
    )

    if compare_res['run result code'] == '0':
        compare_msg_f = file(compare_msg_file, 'r')
        compare_msg = compare_msg_f.read().strip()
        compare_msg_f.close()
        if setting['compare_special']:
            if compare_msg.find('\n') >= 0:
                score, comment = compare_msg.split('\n', 1)
            else:
                score = compare_msg
                comment = ''
            try:
                return (float(score), comment.strip())
            except:
                return (0, ('Compare Result Presentation Error: "%s"' % compare_msg))
        else:
            ret = { 'AC': (1, 'Accepted'), 'PE': (0, 'Presentation Error'), 'WA': (0, 'Wrong Answer') }
            return ret[compare_msg] if compare_msg in ret.keys() else (0, compare_msg)
    else:
        return (0, 'Compare: ' + compare_res['run result'])

def judge(lang, setting):
    program_stdout_file = '__program_stdout_file__'
    time_remain = setting['time_limit_global']
    case_res = []
    global_res = {'time_used': 0, 'mem_used': 0, 'score': 0, 'comment': 'Accepted', 'sum_weight': 0}
    for i in range(0, setting['test_round_count']):
        global_res['sum_weight'] += setting['round_weight'][i]
    for i in range(0, setting['test_round_count']):
        round_res = judge_process_monitor(
            command = run_command[lang],
            time_limit = min(setting['time_limit_case'][i], time_remain),
            mem_limit = setting['memory_limit'],
            process_limit = setting['process_limit'],
            uid = TEST_PLAYER,
            gid = TEST_PLAYER,
            stdin_file = setting['standard_input_files'][i],
            stdout_file = program_stdout_file,
        )
        round_res['score'] = 0
        round_res['comment'] = round_res['run result']
        if round_res['run result code'] == '0':
            round_res['score'], round_res['comment'] = compare(setting['standard_input_files'][i],
                setting['standard_output_files'][i], program_stdout_file, setting)

        time_used_this_round = int(round_res['time used'])
        time_remain -= time_used_this_round
        global_res['time_used'] += time_used_this_round
        global_res['mem_used'] = max(global_res['mem_used'], int(round_res['mem used']))
        global_res['score'] += (round_res['score'] * setting['round_weight'][i] * 1.0) / global_res['sum_weight']
        if round_res['score'] < 1:
            global_res['comment'] = round_res['comment']
        case_res.append(round_res)
    return (global_res, case_res)

def report_judge_result(setting, global_res, case_res):
    round_info = []
    if case_res.__len__() > 1:
        for i in range(0, case_res.__len__()):
            round_res = case_res[i]
            case_str = '%s | %s * (%d / %d) | %s ms | %s KB' % (round_res['comment'], round_res['score'].__str__(),
                setting['round_weight'][i], global_res['sum_weight'], round_res['time used'], round_res['mem used'], )
            round_info.append(case_str)
    report(global_res['score'], global_res['time_used'], global_res['mem_used'], global_res['comment'], '\n'.join(round_info))

if __name__ == '__main__':
    try:
        setting = trim_setting()
        lang = get_lang()
        prepare_special_compare()
        prepare_srcfile(lang, setting)
        compiler(lang, setting)
        report_judge_result(setting, *judge(lang, setting))
    except Exception, e:
        traceback.print_exc()
        estr = type(e), ':', e.__str__()
        report_err(estr)

