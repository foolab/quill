#!/usr/bin/env python

############################################################################
##
## Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
## Contact: Alexander Bokovoy <alexander.bokovoy@nokia.com>
##
## This file is part of the Quill package.
##
## Commercial Usage
## Licensees holding valid Qt Commercial licenses may use this file in
## accordance with the Qt Commercial License Agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and Nokia.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 as published by the Free Software
## Foundation and appearing in the file LICENSE.LGPL included in the
## packaging of this file.  Please review the following information to
## ensure the GNU Lesser General Public License version 2.1 requirements
## will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, Nokia gives you certain
## additional rights. These rights are described in the Nokia Qt LGPL
## Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
## package.
##
## GNU General Public License Usage
## Alternatively, this file may be used under the terms of the GNU
## General Public License version 3.0 as published by the Free Software
## Foundation and appearing in the file LICENSE.GPL included in the
## packaging of this file.  Please review the following information to
## ensure the GNU General Public License version 3.0 requirements will be
## met: http://www.gnu.org/copyleft/gpl.html.
##
## If you are unsure which license is appropriate for your use, please
## contact the sales department at qt-sales@nokia.com.
##
############################################################################

import os
import subprocess
import time
import threading
import datetime
import sys
import signal
import logging as log
import getopt
import zipfile
import pprint

from xml.etree.ElementTree import parse
import xml.etree.ElementTree as ET
from exceptions import Exception

TEST_DEFINITION_PATH = '/usr/share' #default path to test definition file
TEST_RESULTS_PATH = '/tmp'
TEST_INPUT_FILE = None #Optional input and output files, specified from the command prompt
TEST_OUTPUT_FILE = None
TEST_OUTPUT_LOGFILE = None
COLLECT_LOGS_FROM_DIR = '/tmp/logs/'

environment="hardware"

DEFAULT_TIMEOUT = 90.0
MIN_TIMEOUT = 5.0
KILL_TIMER = 5.0

STATUS_NOT_STARTED = "not-started"
STATUS_STARTED = "started"
STATUS_TIMEOUT = "timeout"
STATUS_TERMINATED = "terminated"
STATUS_OK = "ok"
PRINT_OUT_EXECUTION = False
EXEC_ONLY_AUTOMATIC = False

TEST_RESULT_VALUES = {'p' : 'Passed', 'f' : 'Failed', 'n' : 'Not executed'}

LOG_FORMAT = '[%(levelname)s] %(asctime)s %(message)s'
LOG_DATE_FORMAT = '%H:%M:%S'
log.basicConfig(level=log.DEBUG, format=LOG_FORMAT, datefmt=LOG_DATE_FORMAT, stream=sys.stdout)


# Custom exceptions:
class NoTestCaseCommandSpecifiedError(Exception):
    pass

class NoTestCaseIdSpecifiedError(Exception):
    pass

class NoTestCaseSpecifiedError(Exception):
    pass

class NoTestCaseHeaderInfoAvailable(Exception):
    pass




class TestCase(object):
    """A data structure class for a test case"""
    def __init__(self, name = None, requirements = None, command_parameters=[], timeout = DEFAULT_TIMEOUT, description = None, steps = None, expected_results=[], testset=None, subfeature=None, case_type=None):
        # unique test case name inside the test package
        self.name = name
        
        self.requirements = requirements

        #Combine command and parameters as a list for subprocess to process them
        self.command_parameters=command_parameters

        self.timeout = timeout
        self.description = description
        self.steps = steps
        self.expected_results=expected_results

        #to indicate to which test set this test case belongs
        self.testset=testset
        
        self.subfeature=subfeature
        self.type = case_type
        

class TestSet(object):
    """A data structure class for a test set"""
    def __init__(self, name = None, description = None, feature = None, timeout = None, set_type = None, CI = True):
    
        self.name=name
        self.description=description
        self.feature=feature
        self.timeout=timeout
        self.set_type=set_type
        
        # CI is set as True by default
        self.CI=CI
        

class TestRun(object):
    """
    A testcase specific testrun class. Constructor takes test_case object as a parameter.
    run() executes the testcase. 
    """
    def __init__(self, test_case):
        self.test_case = test_case
        self.process = None
        # to store a bunch of stdout and stderr in case there are multiple steps in test case
        self.stdout = []
        self.stderr = []
        self.return_code = []
        
        self.status = STATUS_NOT_STARTED
        self.execution_time = 0.0
        self.started_on = None
        self.end_on = None
        # indicate the final result of test case execution
        self.result = None
        # indicate the result step by step in test case
        self.resultcode=[]
        self.lastIndex = 0

        if not self.test_case:
            log.error('No TestCase specified for TestRun!')
        else:
            log.info('New TestRun (test_case_requirement = %s)' % self.test_case.requirements)

    def run(self):
        if not self.test_case:
            raise NoTestCaseSpecifiedError
        #in case no command is specified, lets skip this function
        if not self.test_case.command_parameters:
            self.status = STATUS_OK
            stdout=''
            self.stdout.append(stdout)
            stderr=''
            self.stderr.append(stderr)
            self.return_code.append(1)
            self.resultcode.append(-1)
            self.started_on = time.strftime("%Y-%m-%d %H:%M:%S")
            return

        self.started_on = time.strftime("%Y-%m-%d %H:%M:%S")

        log.info('Starting test run for test case with name "%s" at %s' % (self.test_case.name, str(self.started_on)))
        
        starting_time = time.time()
 
        index=0; # variable index used to position the expected result for each pair of cmd_param in the test case
            
        #each pair of command and its parameters are passed in as a string to subprocess for processing
        for command_and_params in self.test_case.command_parameters:
            if PRINT_OUT_EXECUTION:
                print ' '.join(command_and_params)
            
            try:
                self.process = subprocess.Popen(command_and_params,
                                                preexec_fn = os.setsid,
                                                stdin = None,
                                                stdout = subprocess.PIPE,
                                                stderr = subprocess.PIPE,
                                                close_fds = True,
                                                shell=True)
            except Exception, e:
                    error = 'Error with command:' + command_and_params+ ' Error: '+ str(e)
                    log.error(error)
                    self.status = STATUS_OK
                    
                    stdout = ''
                    self.stdout.append(stdout)
                    stderr = ''
                    self.stderr.append(stderr)
                    self.return_code.append(1)
                    self.lastIndex=index
                    self.resultcode.append(-1)
                    return

            self.status = STATUS_STARTED

            stdout=''
            stderr=''
            # wait for completion of command execution and get its output or error info.
            stdout,stderr = self.process.communicate()
            self.stdout.append(stdout)
            self.stderr.append(stderr)
                
            if PRINT_OUT_EXECUTION:
                print stdout

            return_code = self.process.returncode
            self.return_code.append(return_code)
# In case there are multiple commands in one test case and one of them fails to execute, it's treated as a failure of the test case as a whole.

            if self.test_case.expected_results[index] != None:
                if return_code != int(self.test_case.expected_results[index]):
                    self.lastIndex=index
                    self.resultcode.append(-1)
                    break
            # if no expected result is defined, 0 is taken as the default value
            elif return_code != 0:
                self.lastIndex = index
                self.resultcode.append(-1)
                break
            self.resultcode.append(0)
            self.lastIndex=index
            index+=1

        # Keep the execution_time attribute as it might turn out to be useful later
        self.execution_time = time.time() - starting_time

        self.end_on = time.strftime("%Y-%m-%d %H:%M:%S")
        self.status = STATUS_OK
        log.info('Finished test run, return code = %d, execution time = %f' % (self.return_code[self.lastIndex], self.execution_time))


class PrestepRun(object):
    """This class runs the commands if any found inside pre_steps tag in the test definition file"""
    def __init__(self, prestepCmd):
        self.timeout_timer = None
        self.kill_timer = None
        
        self.prestepCmd=prestepCmd
        self.process=None
        self.return_code=[]
        
        # record 'daemon' process, if any
        self.subprocesses=[]
        
    def _timeout_callback(self, test_run):
        if not test_run.process:
            self.timeout_timer = None
            return

        if test_run.status == STATUS_OK:
            self.timeout_timer = None
            return

        os.kill(test_run.process.pid, signal.SIGTERM)
        test_run.state = STATUS_TIMEOUT
        self.kill_timer = threading.Timer(KILL_TIMER, self._kill_callback, [test_run])
        self.timeout_timer = None

        log.info('Timeout occurred! Trying to terminate the process ...')

    def _kill_callback(self, test_run):
        os.kill(test_run.process.pid, signal.SIGKILL)

        test_run.state = STATUS_TERMINATED
        self.kill_timer = None

        log.info('Kill timer expired! Killed the process forcefully ...')    
        
    def run(self):
        if not self.prestepCmd:
            self.return_code.append(1)
            return
            
        for cmd_parameters in self.prestepCmd:
            # check whether is it required to run in background
            if cmd_parameters.find('&') != -1:
                # in case there are multiple commands in one step, split them by ';'
                command_list = cmd_parameters.split(";")
                for single_command in command_list:
                    # daemon process execution
                    if single_command.find('&') != -1:
                        try:
                            process = subprocess.Popen(single_command,
                                                   preexec_fn = os.setsid,
                                                   stdin = None,
                                                   stdout = subprocess.PIPE,
                                                   stderr = subprocess.PIPE,
                                                   close_fds = True,
                                                   shell=True)
                            # store this child process object for future reference
                            self.subprocesses.append(process)
                        except Exception, e:
                            error = 'Error with pre-step command:' + cmd_parameters+' Error: '+str(e)
                            log.error(error)
                            return
                        log.info("Pre-step has been executed.")
                    # non-daemon process execution
                    else:
                        try:
                            self.process = subprocess.Popen(single_command,
                                                            preexec_fn = os.setsid,
                                                            stdin = None,
                                                            stdout = subprocess.PIPE,
                                                            stderr = subprocess.PIPE,
                                                            close_fds = True,
                                                            shell=True)
                        except Exception, e:
                            error = 'Error with pre-step command:' + cmd_parameters+ ' Error: '+ str(e)
                            log.error(error)
                            self.return_code.append(1)
                            return
                        
                        # Todo, maybe needed in future
                        stdout=""
                        stderr=""
                        stdout, stderr = self.process.communicate()
            
                        #return_code = self.process.returncode
                        #self.return_code.append(return_code)
                        log.info("Pre-step has been implemented...")
            # normal test environment construction in pre-steps
            else:
                try:
                    self.process = subprocess.Popen(cmd_parameters,
                                                    preexec_fn = os.setsid,
                                                    stdin = None,
                                                    stdout = subprocess.PIPE,
                                                    stderr = subprocess.PIPE,
                                                    close_fds = True,
                                                    shell=True)
                except Exception, e:
                    error = 'Error with pre-step command:' + cmd_parameters+ ' Error: '+ str(e)
                    log.error(error)
                    self.return_code.append(1)
                    return
                
                stdout=""
                stderr=""
                stdout, stderr = self.process.communicate()
            
                return_code = self.process.returncode
                self.return_code.append(return_code)
                log.info("Pre-step has been implemented.")            

class TestParser(object):
    """All the parse work of test definition file is conducted in this class"""
    def __init__(self, statistics, testinputfile, testoutputfile=None):
        self.test_cases = []
        self.testcase_ids = []
        
        # additional files if any to be fetched
        self.additionalFiles = []
        
        # a dict to store all the info of test sets inside the suite
        self.testset = dict()
        
        # a dict to store all the potential pre-step commands, its key is the set index while the value is the command list
        self.prestepCmd = dict()
        
        #setIndex is used to index each test set inside a test package
        self.setIndex=0
        
        # indicate whether to collect statistics from test cases
        self.statistics = statistics
        
        # stack potential 'daemon' process from pre-steps in a test set
        self.child_process = []
        
        self.testinputfile = testinputfile
        self.testoutputfile = testoutputfile
        #headerinfo for final results
        #self.headerinfo = {'name':'', 'version':'', 'comment':'',
        #                   'usedhardware':''}
        self.headerinfo = {'name':'', 'version':'', 'domain':'', 'type':'', 'timeout':'', 'CI':''}
                            
    def start_parsing(self):
        if self.testinputfile:
        #if TEST_INPUT_FILE: #if user-specified input file is used, otherwise use all files from the TEST_DEFINITION_PATH
            # check the condition whether it is to collect statistics or to execute test cases
            if self.statistics:
                self._collect_statistics(self.testinputfile, self.testoutputfile)
            else:
                try:
                    self._parse_test_cases(self.testinputfile, environment)
                except Exception, e:
                    log.error('Error while opening file ' + self.testinputfile + ' ' + str(e))
                    sys.exit(1)
        else:
            self._parse_test_files()

    def _parse_test_files(self):
        files = [f for f in os.listdir(TEST_DEFINITION_PATH) if f.endswith('.xml')]
        log.info('Found test definition files: %s' % ' '.join(files))

        for f in files:
            self._parse_test_cases(TEST_DEFINITION_PATH + os.sep + f, environment)

    # Fetch all the attributes of a tag
    def _xml_element_get_items(self, element):
        values = element.items()
        #pprint.pprint(values)
        items = {}
        for key, value in values:
            items[key] = value

        return items

# return a value of attribute from case, set or suite where value from case is perferred over the one from set while the value in suite has the least priority.
    def _get_value_from_case_or_set(self, key, case, set, subset):
        if case.has_key(key):
            return case[key]
        elif subset.has_key(key):
            return subset[key]
        elif set.has_key(key):
            return set[key]
        elif self.headerinfo.has_key(key):
            return self.headerinfo[key]
        else:
            return None

# Parse the environment tags in definition file and compare it with that passed from command line. 
# Return the value of test environment or None if unmatched.
    def _parse_environment_tags(self, setNode, pattern, environment):
        testenvironment=""
        
        # find search pattern from node set
        environmentsList = setNode.findall(pattern)
        if environmentsList:
            for env in environmentsList:
                flag=0
                for envelement in env:
                    if envelement.tag == environment:
                    # set test environment if it is labeled as true or None if false otherwise 
                        if envelement.text == 'true':
                            testenvironment = environment
                        else:
                            testenvironment = None
                        flag=1
                        break
                # if the specified environment is not in the list of environments in exceptional case, set it as None
                if flag == 0:
                    testenvironment = None
        # test set is assumed to be executed in every environment by default
        else:
            testenvironment = environment            
        return testenvironment
        
# Parse the get tags in definition file
# Return the list of files to be fetched
    def _parse_get_tags(self, getNode):
        fileList=[]
        for fileNode in getNode.getchildren():
            getFileName=fileNode.text
            
            # add the file inside a set to list, file check will be done later
            fileList.append(getFileName)
                
        return fileList
    
                    
# parse pre steps tags
# Return the list of pre-step commands as strings
    def _parse_presteps_tags(self, prestepNode):
        prestepcmd=[]
    
        for stepNode in prestepNode.getchildren():
            stepcmd=stepNode.text
            prestepcmd.extend([stepcmd])
            
        return prestepcmd    
                
# pre-steps setup
    def _setup_presteps(self, prestepRun):
        #prestepRun.timeout_timer = threading.Timer(DEFAULT_TIMEOUT, prestepRun._timeout_callback, [prestepRun])
        #prestepRun.timeout_timer.start()
                    
        prestepRun.run()
        
        # potential 'daemon' process that needs to be terminated after test execution
        if prestepRun.subprocesses:
            self.child_process = prestepRun.subprocesses
                    
        #if prestepRun.kill_timer:
        #    prestepRun.kill_timer.cancel()
        #    prestepRun.kill_timer = None
        #if prestepRun.timeout_timer: # cancel timer after execution in case it is still running
        #    prestepRun.timeout_timer.cancel()      
    
    # configure the headerinfo by fetching attributes from suite tag
    def _config_headerinfo(self, suite):
        name=suite.get('name')
        if name==None:
            log.error ("Name could not be extracted from the XML's suite level. Please correct syntax.")
            raise NoTestCaseHeaderInfoAvailable

        self.headerinfo['name']=name
    
        if suite.get('domain'): self.headerinfo['domain']=suite.get('domain')
        if suite.get('type'): self.headerinfo['type']=suite.get('type')
        if suite.get('timeout'): self.headerinfo['timeout']=suite.get('timeout')
        if suite.get('CI'): self.headerinfo['CI']=suite.get('CI')
        
    
    # configure the set info by fetching attributes from set tag    
    def _config_setinfo(self, setNode, setIndex, set_values):
        # Store each test set's info in a dict
        self.testset[setIndex]=TestSet()
        self.testset[setIndex].name=set_values['name']
        self.testset[setIndex].description= 'description' #set_values['description']
        if set_values.has_key('feature'): self.testset[setIndex].feature=set_values['feature']
        if set_values.has_key('type'): self.testset[setIndex].set_type=set_values['type']
        if set_values.has_key('timeout'): self.testset[setIndex].timeout=set_values['timeout']
    
    
# Function to parse test cases in the definition file 
    def _parse_test_cases(self, filename, environment):
        f=open(filename, "r")

        parser = ET.XMLParser() #some parameters to set to get \n marks to lines??
        xml = parse(f, parser)
        f.close()

        root = xml.getroot()
        version = root.get('version')

        if version==None:
            log.error ("Version couldn't be extracted from the XML's root level. Please correct syntax.")
            raise NoTestCaseHeaderInfoAvailable

        self.headerinfo['version'] = version
        
        # prepare for pre-steps setup if any
        suite_presteps=[]
        set_presteps=[]
                
        for suite in root.getchildren():
            # config headerinfo according to the attribute values fetched from suite tag
            self._config_headerinfo(suite)
            
            # evaluate CI attribute value in suite tag and ignore all tests if CI is set as false
            if not self.headerinfo['CI'] == 'false':
                for set in suite.getchildren():
                    if set.tag == 'pre_steps':
                        suite_presteps = self._parse_presteps_tags(set)
                        
                    
                        # execute pre steps
                        prestepSuite = PrestepRun(suite_presteps)
                        self._setup_presteps(prestepSuite)
                                    
                    elif set.tag == 'set':
                        set_values = self._xml_element_get_items(set)
                    
                        # store the test environment value in dictionary set_values
                        set_values['environment'] = self._parse_environment_tags(set,"environments", environment)           
                    
                        # skip the entire test set if its test environment does not match the one from command line
                        if set_values['environment'] == environment:
         
                            # evaluate CI attribute value in set tag                   
                            test_set_CI = None
                            if set_values.has_key('CI'): test_set_CI = set_values['CI']
                    
                            if not test_set_CI == 'false':
                                # increment the index of test set
                                self.setIndex+=1

                                # config each set's info according to the manually defined set index 
                                self._config_setinfo(set, self.setIndex, set_values)
                                
                                for element in set.getchildren(): #check all the following test cases in this set
                                    if element.tag == 'pre_steps':
                                        set_presteps = self._parse_presteps_tags(element)
                                        
                                        # create a new entry in dict for the pre-steps commands
                                        self.prestepCmd[self.setIndex]=set_presteps
                                
                                        # reset the list for the next iteration
                                        set_presteps=[]
                            
                                    elif element.tag == 'case':
                                        case_values = self._xml_element_get_items(element)
                                        
                                        # evaluate CI attribute value in case tag
                                        test_case_CI = None
                                        if case_values.has_key('CI'): test_case_CI = case_values['CI']
                                        if not test_case_CI == 'false':
                                            cmd_params_list=[]
                                            expected_results=[]
                                            for step in element.getchildren():
                                                expected_results.append(step.get('expected_result'))
                                                cmd_param=step.text

                                                # add each command as a string to the list
                                                cmd_params_list.extend([cmd_param])

                                            case_values['command_parameters']=cmd_params_list
                                            case_values['expected_results']=expected_results
                                            case_values['set_No']=self.setIndex

                                            self._create_testcase(case_values, set_values)
                                    # environments tags are neglected since they were processed before
                                    elif element.tag == "environments":
                                        pass
                                
                                    # parse get tag if any and add files to the global list of files   
                                    elif element.tag == "get":
                                        getFiles=self._parse_get_tags(element)
                                        self.additionalFiles.extend(getFiles)
                                    
                                    else:
                                        log.error('Incorrect tag in the XML: '+ element.tag)                 

#Changed the test case creation function to suit new format
    def _create_testcase(self, case_values, set_values, subset_values={}):
        testcase = TestCase()
        testcase.name = case_values['name'].strip()
        testcase.requirements = self._get_value_from_case_or_set('requirement', case_values, set_values, subset_values)
        testcase.command_parameters=self._get_value_from_case_or_set('command_parameters',case_values, set_values, subset_values)

        testcase.timeout = self._get_value_from_case_or_set('timeout', case_values, set_values, subset_values)

        testcase.description = self._get_value_from_case_or_set('description', case_values, set_values, subset_values)
        testcase.steps = self._get_value_from_case_or_set('steps', case_values, set_values, subset_values)
        testcase.expected_results=self._get_value_from_case_or_set("expected_results", case_values, set_values, subset_values)

        # this attribute relates the specific test case to its test set for grouping purpose
        testcase.testset=self._get_value_from_case_or_set("set_No", case_values,set_values,subset_values)
        
        testcase.subfeature=self._get_value_from_case_or_set("subfeature", case_values, set_values, subset_values)
        
        if case_values.has_key("type"):
            testcase.type=self._get_value_from_case_or_set("type", case_values, set_values, subset_values)

        #do some checkings, could be done in more clever way also...
	#if there are multiple requirements in one test case, they are separated by ','
        if testcase.requirements:
            testcase.requirements = testcase.requirements.split(',')
        # specify the test case execution timeout, defaults to 90.0 if it is not specified explicitly
        try:
            testcase.timeout = float(testcase.timeout)
        except:
            testcase.timeout = DEFAULT_TIMEOUT

        if testcase.name not in self.testcase_ids:
            self.test_cases.append(testcase)
            self.testcase_ids.append(testcase.name)
        else:
            log.error('There is a duplicate test case with name:' + testcase.name )
            sys.exit(1)
            
    # function to collect test cases information
    def _collect_statistics(self, readfilename, writefilename):
        pass



class TestRunner(object):
    """TestRunner class handles the whole testrunning process"""
    def __init__(self, test_input_file, test_output_file):
        self.timeout_timer = None
        self.kill_timer = None
        
        self.testinputfile = test_input_file
        self.testoutputfile = test_output_file

        self.test_execution_info_text = ' '.join([v.upper() + ' (' + r + ')' for v,r in TEST_RESULT_VALUES.iteritems()])
        self.totalcounter = 0 #calc executed testcase amount
        self.counters = {} #for counting results for a "simple" summary. Next line sets proper counters to zero
        for k in TEST_RESULT_VALUES.keys(): self.counters[k] = 0


    def _timeout_callback(self, test_run):
        if not test_run.process:
            self.timeout_timer = None
            return

        if test_run.status == STATUS_OK:
            self.timeout_timer = None
            return

        os.kill(test_run.process.pid, signal.SIGTERM)
        test_run.state = STATUS_TIMEOUT
        self.kill_timer = threading.Timer(KILL_TIMER, self._kill_callback, [test_run])
        self.timeout_timer = None

        log.info('Timeout occurred! Trying to terminate the process ...')

    def _kill_callback(self, test_run):
        os.kill(test_run.process.pid, signal.SIGKILL)

        test_run.state = STATUS_TERMINATED
        self.kill_timer = None

        log.info('Kill timer expired! Killed the process forcefully ...')

    def _write_temporary_result(self, test_run, parsedTest):
        # group test cases results according to their corresponding test set ID
        filename = TEST_RESULTS_PATH + os.sep + parsedTest.headerinfo['name']+os.sep+str(test_run.test_case.testset)+os.sep+str(test_run.test_case.name)+'.tmp'
        try:
            file = open(filename, 'w')
        except:
            test_results_path = TEST_RESULTS_PATH+os.sep+parsedTest.headerinfo['name']+os.sep+str(test_run.test_case.testset)
            if not os.path.isdir(TEST_RESULTS_PATH+os.sep+parsedTest.headerinfo['name']):
                try:
                    os.mkdir(TEST_RESULTS_PATH+os.sep+parsedTest.headerinfo['name'])
                except:
                    log.error("Cannot create directory "+TEST_RESULTS_PATH+os.sep+parsedTest.headerinfo['name'])
            os.mkdir(test_results_path)
        file=open(filename, 'w')

# start to build the xml file with ET
        root = ET.Element("case")
        root.set("name", test_run.test_case.name)
        if test_run.test_case.subfeature:
            root.set("subfeature", str(test_run.test_case.subfeature))
        if test_run.test_case.type:
            root.set("type", str(test_run.test_case.type))
        
        if test_run.resultcode[test_run.lastIndex] != 0:
            root.set("result", "FAIL")
        else:
            root.set("result", "PASS")
        if test_run.test_case.description != None:
            root.set("description", test_run.test_case.description)
        else:
            root.set("description", "")
        if test_run.test_case.requirements != None:
            root.set("requirement", str(test_run.test_case.requirements))
        else:
            root.set("requirement", "")

        i=0
        # start to build the step tag one by one. It won't stop until it encounters the last index of step executed by the subprocess
        while i<=test_run.lastIndex:

            step = ET.SubElement(root, "step")

            # build step command that is constructed in a list 
            step.set("command", test_run.test_case.command_parameters[i])

            if test_run.resultcode[i] != 0:
                step.set("result", "FAIL")
            else:
                step.set("result", "PASS")

            expect_result=ET.SubElement(step, "expected_result")
            if len(test_run.test_case.expected_results) != 0:
                expect_result.text=test_run.test_case.expected_results[i]
            else:
                expect_result.text=""
            return_code=ET.SubElement(step, "return_code")
            return_code.text=str(test_run.return_code[i])
            
            # todo, the start and end time are now counted as the entire execution time of a test case.
            started_on=ET.SubElement(step, "start")
            started_on.text=str(test_run.started_on)
            end_on=ET.SubElement(step, "end")
            end_on.text=str(test_run.end_on)

#       Uncomment the following two lines if execution time needs to be shown in the result file
#       execution_time=ET.SubElement(step, "execution_time")
#       execution_time.text=str(test_run.execution_time)

            stdout=ET.SubElement(step,"stdout")
            stdout.text=test_run.stdout[i]
            stderr=ET.SubElement(step,"stderr")
            stderr.text=test_run.stderr[i]
            
            i+=1

        tree=ET.ElementTree(root)
        tree.write(filename)

        file.close()

    def _get_file_contents(self, filename):
        file = open(filename, 'r')
        contents = file.read()
        file.close()
        return contents

    def _write_final_result(self, parsedTest):
        test_results_path=TEST_RESULTS_PATH+os.sep+parsedTest.headerinfo['name']

        temp_results=""

#       Variable sindex positions the test sets. It starts from the first set in the test package.       
        sindex=1

#       Loop through each test set and group the test results according to test set
        while sindex != parsedTest.setIndex+1:
            firstoccurrance=0
            for f in os.listdir(test_results_path+os.sep+str(sindex)):
                if(firstoccurrance==0):
                    temp_results+='<set name="'
                    #temp_results+= parsedTest.testset_name[sindex]
                    temp_results+= parsedTest.testset[sindex].name
                    temp_results+='" description="'
                    #temp_results+=parsedTest.testset_description[sindex]
                    temp_results+= parsedTest.testset[sindex].description
                    if parsedTest.testset[sindex].feature:
                        temp_results+='" feature="'
                        temp_results+=parsedTest.testset[sindex].feature
                    if parsedTest.testset[sindex].set_type:
                        temp_results+='" type="'
                        temp_results+=parsedTest.testset[sindex].set_type
                    temp_results+='" environment="'
                    temp_results+=environment
                    temp_results+='">'
                    temp_results+="\n"
                    firstoccurrance=1
                temp_results+=self._get_file_contents(test_results_path + os.sep +str(sindex)+os.sep+ f)
            temp_results+="\n</set>\n"
            sindex+=1

        # prepare header info
        header = ''
        header1 = ''
        header += ' ' + 'version' + '="' + str(parsedTest.headerinfo['version']) + '"'
        header1 += ' ' + 'name' + '="' + str(parsedTest.headerinfo['name']) + '"'
        if parsedTest.headerinfo['domain']: header1+=' '+ 'domain'+'="'+str(parsedTest.headerinfo['domain'])+'"'
        if parsedTest.headerinfo['type']: header1+=' '+'type'+'="'+str(parsedTest.headerinfo['type'])+'"'
        
        if self.testoutputfile:
            outputfile = self.testoutputfile
        else:
            # Hard-coded result file name if user does not specify an output file name
            outputfile = test_results_path + os.sep + 'tatam_xml_testrunner_output_results.xml'
            
        # Fetch the directory name of the output file.
        result_path = os.path.dirname(outputfile)
        
        # Try to create the directory if it is not existent
        if not os.path.exists(result_path) and result_path != '':
            try:
                os.mkdir(result_path)
            except:
                log.error("Cannot create directory "+result_path)

        file = open(outputfile, 'w')
        file.write('<?xml version="1.0" encoding="ISO-8859-1"?>\n')
#       file.write('<?xml-stylesheet type="text/xsl" href="http://cita.research.nokia.com/static/testrunner_results.xsl"?>')
        file.write('<testresults'+header+'>\n')
        file.write('<suite'+header1+'>\n')
        file.write(temp_results)
        file.write('</suite>\n')
        file.write('</testresults>\n')
        file.close()
        log.info('Results were written to: ' + outputfile)

        #delete temporary files and directories
        while parsedTest.setIndex > 0:
            temp_results_files = [f for f in os.listdir(test_results_path+os.sep+str(parsedTest.setIndex)) if f.endswith('.tmp')]
            for f in temp_results_files:
                os.unlink(test_results_path + os.sep + str(parsedTest.setIndex)+ os.sep + f)
            os.rmdir(test_results_path + os.sep + str(parsedTest.setIndex))
            parsedTest.setIndex-=1

        self._collect_logs_to_zipfile()
        self._print_total_counters()

    def _print_total_counters(self):
        log.info('Executed ' +str(self.totalcounter) +' cases where '+ ' '.join([TEST_RESULT_VALUES[k]+':' + str(v) for k, v in self.counters.iteritems()]))

    def _collect_logs_to_zipfile(self):
        if not os.path.isdir(COLLECT_LOGS_FROM_DIR):
            #log.info('No logs to collect from ' + COLLECT_LOGS_FROM_DIR)
            return

        if TEST_OUTPUT_LOGFILE:
            zipfilename = TEST_OUTPUT_LOGFILE
        else:
            zipfilename = TEST_RESULTS_PATH + os.sep + 'results_log.zip'
        log.info('Copying log files from '+ COLLECT_LOGS_FROM_DIR +' to a zip file: '+zipfilename)
        zfile = zipfile.ZipFile(zipfilename, 'w')
        for root, dirs, files in os.walk(COLLECT_LOGS_FROM_DIR):
            for fileName in files:
                zfile.write(os.path.join(root,fileName))
        zfile.close()
        
    # Fetch all the additional files if any        
    def _fetch_get_files(self, parsedTest):
        
        # check the existence of target directory, in case it is not existent, create it.
        result_path = os.path.dirname(self.testoutputfile)
        
        if not os.path.isdir(result_path) and result_path != '':
            try:
                os.mkdir(result_path)
            except:
                log.error("Cannot create directory "+result_path)
        
        # copy additional files, if any, to designated directory
        if parsedTest.additionalFiles:
            #targetDir="/tmp/"+parsedTest.headerinfo['name']
            targetDir = result_path
            for eachFile in parsedTest.additionalFiles:
                # evaluate the existance of files in the list
                #if os.path.isfile(eachFile):
                try:
                    subprocess.call("cp" + " "+eachFile+" "+targetDir, shell = True)
                except OSError, e:
                    print >>sys.stderr, "Copy file "+eachFile+" failed:", e


    def run_single_case(self, test_case, parsedTest):
        if not test_case:
            raise NoTestCaseSpecifiedError


        r = TestRun(test_case)
        self.timeout_timer = threading.Timer(test_case.timeout, self._timeout_callback, [r])
        self.timeout_timer.start()

        r.run()

        if self.kill_timer:
            self.kill_timer.cancel()
            self.kill_timer = None
        if self.timeout_timer: #Cancel timer after execution in case it is still running.
            self.timeout_timer.cancel()

        #If there is no final result available, analyze result. In case manual exec, this analysis will be overidden in next step
        if not r.result:
            r.result = self._analyze_result(r)

        self._write_temporary_result(r, parsedTest)
        return r.result

    def run_all_cases(self):
        log.info('Starting to run tests ...')
        #log.info('(Not implemented) Clearing log directory ' + COLLECT_LOGS_FROM_DIR)
        
        # Inside test runner, there is absolutely no opportuniry to collect test cases statistics
        parsedTest = TestParser(False, self.testinputfile)
        parsedTest.start_parsing()

        setCount=1
        while setCount != parsedTest.setIndex+1:
            # to check whether the specific test set has any pre-steps commands to be executed first
            if parsedTest.prestepCmd.has_key(setCount):
                set_presteps=parsedTest.prestepCmd[setCount]
                prestepSet=PrestepRun(set_presteps)
                parsedTest._setup_presteps(prestepSet)
            
            # execute test cases in this test set followingly            
            for case in parsedTest.test_cases:
                if case.testset == setCount:
                    result = self.run_single_case(case, parsedTest)
                    self.counters[result] += 1
                    self.totalcounter += 1
                    
            # terminate any 'daemon' process
            if parsedTest.child_process:
                for each_child in parsedTest.child_process:
                    #print each_child.pid
                    os.kill(each_child.pid, signal.SIGTERM)
            # reset the list of child process for next test set that has such 'daemon' process
            parsedTest.child_process = []
            
            setCount+=1
            
        if parsedTest.test_cases:
            self._write_final_result(parsedTest)
        log.info('Finished!')
      
      # finally, fetch all the additional result files if any to designated directory  
        self._fetch_get_files(parsedTest)

    def ask_result(self, t):
        #ask result from the user in case of manual test execution
        print 'Description: ', t.description, os.linesep
        print 'Steps:'
        #pprint.pprint( t.steps)
        parsedsteps = str(t.steps).replace('#', os.linesep).replace('\n', os.linesep).replace(';', os.linesep).replace('\\n', os.linesep).replace('\\t', '\t').split(os.linesep)
        for line in parsedsteps:
            print ' ' + line
        print 'Please enter result. ' + self.test_execution_info_text +' (The case doesn\'t matter)'
        resp = raw_input("What is the result? ")
        while(self._check_user_input(resp)):
            resp = raw_input("Error in the input, What is the result? ")
        return str(resp).lower()

    def ask_verbal_input(self):
        resp = raw_input("Comment test run (if needed): ")
        return str(resp)

    #In manual execution, check user input.
    def _check_user_input(self, i):
        accepted_inputs = TEST_RESULT_VALUES.keys()
        try:
            if i.lower() in accepted_inputs:
                return 0
            else:
                return 1
        except:
            return 1
        return 0

    #analyze automated test execution result
    def _analyze_result(self, r):
    # the premise for the evaluation on the result of test case execution is that it is treated as pass only when all the steps have been successfully executed. In all the other conditions, it is treated as fail. So it only checks the execution status of last step in the test case. 
        if r.resultcode[r.lastIndex] == 0:
            return 'p'
        return 'f'

if __name__ == '__main__':
    def usage():
        print "The usage of this script is as follows."
        print "Use option -v to print out execution output during execution"
        print "Use -f option to define input test xml file."
        print "Use -o option to define output file for the results."
        print "Use -s option to collect statistics about test cases info. (This will not start executing test cases though.)"
        print "Use -e option to define the environment. (hardware/scratchbox etc.) Default is hardware."
        print "Use -z option to define output file for the zipped logs file. Otherwise logs will be written to ./result directory."
#        print "Use -a option to execute only automated test cases"

# testing environment defaults to 'hardware'
    environment="hardware"
    try:
        # options that can be recognized by the script
        # If -s or --statistics is present, only parsed test cases information will be provided, but test cases will not be executed
        opts, args = getopt.getopt(sys.argv[1:], "hsf:o:z:e:va", ["help", "statistics", "file=", "output=", "zip=", "environment="])
    except getopt.GetoptError, err:
        # print help information and exit
        print str(err) # will print something like "option -a not recognized"
        usage()
        sys.exit(2)

    verbose = False
    statistics = False
    for o, a in opts:
        if o == "-v":
            PRINT_OUT_EXECUTION = True
        elif o in ("-h", "--help"):
            usage()
            sys.exit()
        elif o in ("-s", "--statistics"):
            statistics = True
            print "Collecting statistics now..."
        elif o in ("-f", "--file"):
            TEST_INPUT_FILE = a
            #in case input file is specified and no outputfile specified in opts, outputfile will be named according to it as inputname_results.xml
#            if not TEST_OUTPUT_FILE:
#                TEST_OUTPUT_FILE = TEST_RESULTS_PATH + os.sep + os.path.basename(a).strip('.xml') + '_results.xml'
#            if not TEST_OUTPUT_LOGFILE:
#                TEST_OUTPUT_LOGFILE = TEST_RESULTS_PATH + os.sep + os.path.basename(a).strip('.xml') + '_logs.zip'
        elif o in ("-o", "--output"):
            TEST_OUTPUT_FILE = a
#            try:
#                file = open(TEST_OUTPUT_FILE, 'a') #do not overwrite here, will be overwritten later anyway..
#                file.close()
#            except Exception, e:
#                log.error('Output file name ' + a + ' is not valid ' + str(e))
        elif o in ("-z", "--zip"):
            TEST_OUTPUT_LOGFILE = a
            try:
                file = open(TEST_OUTPUT_LOGFILE, 'a') #do not overwrite here, will be overwritten later anyway..
                file.close()
            except Exception, e:
                log.error('Zip output file name ' + a + ' is not valid ' + str(e))
        elif o == "-a":
            EXEC_ONLY_AUTOMATIC = True
        elif o in ("-e", "--environment"):
            environment=a
        else:
            assert False, "unhandled option"

    if not (TEST_OUTPUT_FILE and TEST_INPUT_FILE): # make input and output file parameters mandatory:
        usage()
        sys.exit(-1)
     
            
    # Run according to the new test package definition if test definition xml is found:
    if os.path.isfile(TEST_INPUT_FILE):
        if statistics:
            testparser = TestParser(statistics, TEST_INPUT_FILE, TEST_OUTPUT_FILE)
            testparser.start_parsing()
        else:
            tr = TestRunner(TEST_INPUT_FILE, TEST_OUTPUT_FILE)
            try:
                tr.run_all_cases()
            except (KeyboardInterrupt, SystemExit):
                #if user exists, write out the results anyway to the file. Let's save what can be saved.
                log.error('Process interrupted. Previous test results were written to the file.')
                parseTest= TestParser(False, TEST_INPUT_FILE, TEST_OUTPUT_FILE)
                parseTest.start_parsing()
                tr._write_final_result(parseTest)
                sys.exit(1)

    # Try to find runtests.sh and run the old style test package
    elif os.path.isfile(TEST_INPUT_FILE.replace("tests.xml", "runtests.sh")):
        log.info("Package seems to use the old test package format. Executing runtests.sh.")
        log.info("Please consider moving to new xml based testpackages.")
        log.info("More info can be found at: https://projects.maemo.org/trac/testautomation/wiki/MaemoTestDefinition")
        
        sys.exit(subprocess.call(TEST_INPUT_FILE.replace("tests.xml", "runtests.sh"), shell=True))

    # Else fail the test
    else:
        log.info("Couln't find tests.xml or runtests.sh.")
        sys.exit(-1)

