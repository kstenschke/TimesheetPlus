#!/usr/bin/env bats

########################################################################################################################
# Test merge command
########################################################################################################################

load test_helper

@test '"d" adds an entry' {
  run $BATS_TEST_DIRNAME/tsp
  [ "$status" -eq 0 ]
  # There are no entries
  run grep -c '<td class="meta">/' $BATS_TEST_DIRNAME/timesheet.html
  [[ "$output" = 0 ]]

  run $BATS_TEST_DIRNAME/tsp d
  [ "$status" -eq 0 ]
  # There is 1 stopped entry now
  run grep -c '<td class="meta">p/' $BATS_TEST_DIRNAME/timesheet.html
  [[ "$output" = 1 ]]
}

@test '"d t=123" adds an entry with task number' {
  run $BATS_TEST_DIRNAME/tsp d t=123
  [ "$status" -eq 0 ]

  run grep -c '<td>123</td>' $BATS_TEST_DIRNAME/timesheet.html
  [[ "$output" = 1 ]]
}

@test '"d 123" adds an entry with task number' {
  run $BATS_TEST_DIRNAME/tsp d t=123
  [ "$status" -eq 0 ]

  run grep -c '<td>123</td>' $BATS_TEST_DIRNAME/timesheet.html
  [[ "$output" = 1 ]]
}

@test '"d c=foo" adds an entry with comment' {
  run $BATS_TEST_DIRNAME/tsp d c=foo
  [ "$status" -eq 0 ]

  run grep -c '<td>foo</td>' $BATS_TEST_DIRNAME/timesheet.html
  [[ "$output" = 1 ]]
}

@test '"d c=\"foo bar\"" adds an entry with comment' {
  run $BATS_TEST_DIRNAME/tsp d c="foo bar"
  [ "$status" -eq 0 ]

  run grep -c '<td>foo bar</td>' $BATS_TEST_DIRNAME/timesheet.html
  [[ "$output" = 1 ]]
}

@test '"d 123 c=\"foo bar\"" adds an entry with task and comment' {
  run $BATS_TEST_DIRNAME/tsp d c="foo bar"
  [ "$status" -eq 0 ]

  run grep -c '<td>123</td>' $BATS_TEST_DIRNAME/timesheet.html
  [[ "$output" = 1 ]]

  run grep -c '<td>foo bar</td>' $BATS_TEST_DIRNAME/timesheet.html
  [[ "$output" = 1 ]]
}

@test 'Adding a full-day entry into a day that has entries already, fails' {
  run $BATS_TEST_DIRNAME/tsp d c="foo bar"
  [ "$status" -eq 0 ]

  $BATS_TEST_DIRNAME/tsp d | grep 'Cannot add full-day entry'
}
