set pagination off
set confirm off

# Result:
# 0 = PASS
# 1 = bad_test_fail()
# 2 = HardFault

# Stop before the RTOS starts so the initial
# bad_test_init(2) write isn't caught by the watchpoint.
break bad_rtos_start
commands
    silent

    printf "Runner starting, installing testcase watchpoint\n"

    watch current_testcase
    commands
        silent
        if current_testcase == 0
            printf "All Tests Passed \n\n"
            quit 0
        else
            printf "Running test : %s \n\n", current_testcase->test_name
            continue
        end
    end
    continue
end

# A task called bad_test_fail()
break bad_test_fail
commands
    silent
    printf "TEST FAIL: bad_test_fail() reached\n\n"
    info symbol current_testcase->test_name
    bt
    quit 1
end

# Cortex-M HardFault
break isr_hardfault
commands
    silent
    printf "TEST FAIL: HardFault_Handler reached\n\n"
    info symbol current_testcase->test_name
    bt
    quit 2
end

# Timeout reached
break timeout_handler
commands
    silent
    printf "TEST FAIL: timeout reached\n\n"
    info symbol current_testcase->test_name
    bt
    quit 2
end

run

printf "TEST FAIL: Target halted unexpectedly\n\n"
bt
quit 2
