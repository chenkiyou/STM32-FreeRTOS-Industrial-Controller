#include "app_state.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static unsigned int s_checks;
static unsigned int s_failures;

#define CHECK(condition)                                                        \
  do                                                                            \
  {                                                                             \
    s_checks++;                                                                 \
    if (!(condition))                                                           \
    {                                                                           \
      s_failures++;                                                             \
      (void)fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__,             \
                    #condition);                                                \
    }                                                                           \
  } while (0)

static void enter_idle(AppStateContext *context)
{
  CHECK(AppState_Init(context) == APP_STATE_OK);
  CHECK(AppState_HandleEvent(context, APP_STATE_EVENT_INIT_COMPLETE) == APP_STATE_OK);
  CHECK(context->state == APP_STATE_IDLE);
}

static void enter_running(AppStateContext *context)
{
  enter_idle(context);
  CHECK(AppState_HandleEvent(context, APP_STATE_EVENT_START) == APP_STATE_OK);
  CHECK(context->state == APP_STATE_RUNNING);
}

static void test_initial_state(void)
{
  AppStateContext context;

  CHECK(AppState_Init(NULL) == APP_STATE_ERR_NULL_CONTEXT);
  CHECK(AppState_Init(&context) == APP_STATE_OK);
  CHECK(context.state == APP_STATE_INIT);
  CHECK(context.fault_clear_confirmed == false);
}

static void test_legal_transitions(void)
{
  AppStateContext context;

  CHECK(AppState_Init(&context) == APP_STATE_OK);
  CHECK(AppState_HandleEvent(&context, APP_STATE_EVENT_INIT_COMPLETE) == APP_STATE_OK);
  CHECK(context.state == APP_STATE_IDLE);
  CHECK(AppState_HandleEvent(&context, APP_STATE_EVENT_START) == APP_STATE_OK);
  CHECK(context.state == APP_STATE_RUNNING);
  CHECK(AppState_HandleEvent(&context, APP_STATE_EVENT_STOP) == APP_STATE_OK);
  CHECK(context.state == APP_STATE_IDLE);
}

static void test_invalid_events_and_transitions(void)
{
  AppStateContext context;

  CHECK(AppState_HandleEvent(NULL, APP_STATE_EVENT_START) == APP_STATE_ERR_NULL_CONTEXT);
  CHECK(AppState_Init(&context) == APP_STATE_OK);
  CHECK(AppState_HandleEvent(&context, (AppStateEvent)999) == APP_STATE_ERR_INVALID_EVENT);
  CHECK(context.state == APP_STATE_INIT);

  CHECK(AppState_HandleEvent(&context, APP_STATE_EVENT_START) == APP_STATE_ERR_INVALID_TRANSITION);
  CHECK(context.state == APP_STATE_INIT);
  CHECK(AppState_HandleEvent(&context, APP_STATE_EVENT_FAULT_DETECTED) == APP_STATE_ERR_INVALID_TRANSITION);
  CHECK(context.state == APP_STATE_INIT);

  CHECK(AppState_HandleEvent(&context, APP_STATE_EVENT_INIT_COMPLETE) == APP_STATE_OK);
  CHECK(AppState_HandleEvent(&context, APP_STATE_EVENT_INIT_COMPLETE) == APP_STATE_ERR_INVALID_TRANSITION);
  CHECK(context.state == APP_STATE_IDLE);
  CHECK(AppState_HandleEvent(&context, APP_STATE_EVENT_STOP) == APP_STATE_ERR_INVALID_TRANSITION);
  CHECK(context.state == APP_STATE_IDLE);
  CHECK(AppState_HandleEvent(&context, APP_STATE_EVENT_FAULT_CLEARED) == APP_STATE_ERR_INVALID_TRANSITION);
  CHECK(context.state == APP_STATE_IDLE);
  CHECK(AppState_HandleEvent(&context, APP_STATE_EVENT_FAULT_RESET) == APP_STATE_ERR_INVALID_TRANSITION);
  CHECK(context.state == APP_STATE_IDLE);

  CHECK(AppState_HandleEvent(&context, APP_STATE_EVENT_START) == APP_STATE_OK);
  CHECK(AppState_HandleEvent(&context, APP_STATE_EVENT_START) == APP_STATE_ERR_INVALID_TRANSITION);
  CHECK(context.state == APP_STATE_RUNNING);
  CHECK(AppState_HandleEvent(&context, APP_STATE_EVENT_INIT_COMPLETE) == APP_STATE_ERR_INVALID_TRANSITION);
  CHECK(context.state == APP_STATE_RUNNING);
}

static void test_fault_latching_and_reset(void)
{
  AppStateContext context;

  enter_running(&context);
  CHECK(AppState_HandleEvent(&context, APP_STATE_EVENT_FAULT_DETECTED) == APP_STATE_OK);
  CHECK(context.state == APP_STATE_FAULT);
  CHECK(context.fault_clear_confirmed == false);

  CHECK(AppState_HandleEvent(&context, APP_STATE_EVENT_START) == APP_STATE_ERR_INVALID_TRANSITION);
  CHECK(context.state == APP_STATE_FAULT);
  CHECK(AppState_HandleEvent(&context, APP_STATE_EVENT_STOP) == APP_STATE_ERR_INVALID_TRANSITION);
  CHECK(context.state == APP_STATE_FAULT);
  CHECK(AppState_HandleEvent(&context, APP_STATE_EVENT_FAULT_RESET) == APP_STATE_ERR_FAULT_NOT_CLEARED);
  CHECK(context.state == APP_STATE_FAULT);

  CHECK(AppState_HandleEvent(&context, APP_STATE_EVENT_FAULT_CLEARED) == APP_STATE_OK);
  CHECK(context.state == APP_STATE_FAULT);
  CHECK(context.fault_clear_confirmed == true);
  CHECK(AppState_HandleEvent(&context, APP_STATE_EVENT_FAULT_RESET) == APP_STATE_OK);
  CHECK(context.state == APP_STATE_IDLE);
  CHECK(context.fault_clear_confirmed == false);
}

static void test_fault_requires_new_clear_after_reentry(void)
{
  AppStateContext context;

  enter_running(&context);
  CHECK(AppState_HandleEvent(&context, APP_STATE_EVENT_FAULT_DETECTED) == APP_STATE_OK);
  CHECK(AppState_HandleEvent(&context, APP_STATE_EVENT_FAULT_CLEARED) == APP_STATE_OK);
  CHECK(AppState_HandleEvent(&context, APP_STATE_EVENT_FAULT_RESET) == APP_STATE_OK);
  CHECK(context.state == APP_STATE_IDLE);

  CHECK(AppState_HandleEvent(&context, APP_STATE_EVENT_START) == APP_STATE_OK);
  CHECK(AppState_HandleEvent(&context, APP_STATE_EVENT_FAULT_DETECTED) == APP_STATE_OK);
  CHECK(context.state == APP_STATE_FAULT);
  CHECK(context.fault_clear_confirmed == false);
  CHECK(AppState_HandleEvent(&context, APP_STATE_EVENT_FAULT_RESET) == APP_STATE_ERR_FAULT_NOT_CLEARED);
  CHECK(context.state == APP_STATE_FAULT);
}

static void test_fault_from_idle_and_repeated_fault(void)
{
  AppStateContext context;

  enter_idle(&context);
  CHECK(AppState_HandleEvent(&context, APP_STATE_EVENT_FAULT_DETECTED) == APP_STATE_OK);
  CHECK(context.state == APP_STATE_FAULT);
  CHECK(AppState_HandleEvent(&context, APP_STATE_EVENT_FAULT_DETECTED) == APP_STATE_OK);
  CHECK(context.state == APP_STATE_FAULT);
  CHECK(context.fault_clear_confirmed == false);
}

static void test_state_names(void)
{
  CHECK(strcmp(AppState_GetStateName(APP_STATE_INIT), "INIT") == 0);
  CHECK(strcmp(AppState_GetStateName(APP_STATE_IDLE), "IDLE") == 0);
  CHECK(strcmp(AppState_GetStateName(APP_STATE_RUNNING), "RUNNING") == 0);
  CHECK(strcmp(AppState_GetStateName(APP_STATE_FAULT), "FAULT") == 0);
  CHECK(strcmp(AppState_GetStateName((AppState)999), "UNKNOWN") == 0);
}

int main(void)
{
  test_initial_state();
  test_legal_transitions();
  test_invalid_events_and_transitions();
  test_fault_latching_and_reset();
  test_fault_requires_new_clear_after_reentry();
  test_fault_from_idle_and_repeated_fault();
  test_state_names();

  (void)printf("Checks: %u, failures: %u\n", s_checks, s_failures);
  return (s_failures == 0U) ? 0 : 1;
}
