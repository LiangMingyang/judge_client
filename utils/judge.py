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
    'python': '',
    'java'  : 'javac Main.java',
    'pascal': 'fpc main.pas',
}

run_command = {
    'c++'   : './main',
    'c'     : './main',
    'python': 'python ./main.py',
    'java'  : 'java Main',
    'pascal': './main',
}

TEST_MASTER = 8193
TEST_PLAYER = 8194

process_monitor_file = '/utils/process_monitor'
compare_file = '/utils/cmp'

def report(score, time_used, mem_used, global_comment, detail_info):
    #repf = file('__report__', 'w')
    #repf.write('%s, %d, %d\n%s' % (score.__str__(), time_used, mem_used, global_comment))
    print '%s, %d, %d\n%s' % (score.__str__(), time_used, mem_used, global_comment)
    if detail_info is not None:
        #repf.write('\n%s' % detail_info)
        print detail_info
    #repf.close()
    sys.exit(0)

def report_err(comment, detail_info = None):
    report(0, 0, 0, comment, detail_info)

def get_lang():
    langf = file('/submission/__lang__', 'r')
    lang = langf.read().strip()
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

    def check_integer_value(value, low, high, err_comment):
        try:
            ret = int(value)
            if (low is not None and ret < low) or (high is not None and ret > high):
                raise Exception('Error', 'Value not in range.')
            return ret
        except:
            config_errs.append(err_comment)

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

    if config_errs.__len__() > 0:
        report_err('Test setting error', '\n'.join(config_errs))
    return real_set

def judge_process_monitor(command, time_limit = 100000, mem_limit = 65535, process_limit = 5,
    uid = TEST_MASTER, gid = TEST_MASTER, cpu_mask = None, stdin_file = "/dev/null", stdout_file = "/dev/null",
    stderr_file = "/dev/null", rejected_syscall = []):
    options = ''
    if time_limit is not None:    options = options + ' -t %d' % time_limit
    if mem_limit is not None:     options = options + ' -m %d' % mem_limit
    if process_limit is not None: options = options + ' -p %d' % process_limit
    if uid is not None:           options = options + ' -uid %d' % uid
    if gid is not None:           options = options + ' -gid %d' % gid
    if cpu_mask is not None:      options = options + ' -cm %d' % cpu_mask
    if stdin_file is not None:    options = options + ' -inf %s' % stdin_file
    if stdout_file is not None:   options = options + ' -outf %s' % stdout_file
    if stderr_file is not None:   options = options + ' -errf %s' % stderr_file
    for rsc in rejected_syscall:  options = options + ' -rsc %d' % rsc
    res_lines = os.popen('%s %s %s' % (process_monitor_file, options, command)).readlines()
    res = {}
    for line in res_lines:
        if line.find(':') >= 0:
            k, v = line.split(':', 1)
            res[k.lower().strip()] = v.strip()
    return res

special_compare_path = '/data/__compare__'

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

def prepare_srcfile(lang, setting):
    if lang not in main_filename.keys():
        report_err('Submission error', 'Language "%s" is not supported' % lang)
    os.system('cp -p /submission/__main__ %s' % main_filename[lang])

def compiler(lang, setting):
    cp_time_limit = 10000 # ms
    cp_mem_limit  = 200 * 1024 # KB
    cp_process_limit = 100
    cp_msg_file   = '__cp_msg__'
    cp_com = compiler_command[lang]
    if cp_com:
        cp_res = judge_process_monitor(
            command = cp_com,
            time_limit = cp_time_limit,
            mem_limit = cp_mem_limit,
            process_limit = cp_process_limit,
            stdout_file = cp_msg_file,
            stderr_file = cp_msg_file,
        )
        if cp_res['run result code'] == '0':
            if cp_res['exit code'] != '0':
                cp_msg_f = file(cp_msg_file, 'r')
                cp_msg = cp_msg_f.read()
                cp_msg_f.close()
                report_err('Compiler Error', cp_msg)
        else:
            report_err('Compiler Error', 'Compiler: ' + cp_res['run result'])

def compare(stdin_file, stdout_file, program_stdout_file, setting):
    if setting['compare_special']:
        compare_command = '%s %s %s %s' % (special_compare_path, stdin_file, stdout_file, program_stdout_file)
    else:
        compare_command = '%s %s %s %d %d %d %d' % (compare_file, stdout_file, program_stdout_file,
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
    compare_res = judge_process_monitor(
        command = compare_command,
        time_limit = compare_time_limit,
        mem_limit = compare_mem_limit,
        process_limit = compare_process_limit,
        stdout_file = compare_msg_file,
        stderr_file = compare_msg_file,
        uid = 0,  #注意，为了防止万能作弊，所有的数据中o权限为0，因此用来比较的比较器有着root权限，谨慎谨慎！
	gid = 0
    )
    '''
    compare_res = judge_process_monitor(
        command = compare_command,
        time_limit = compare_time_limit,
        mem_limit = compare_mem_limit,
        process_limit = compare_process_limit,
        stdout_file = compare_msg_file,
        stderr_file = compare_msg_file,
    )
    '''
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
