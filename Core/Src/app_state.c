/**
  ******************************************************************************
  * @file    app_state.c
  * @brief   Hardware-independent equipment state machine implementation.
  ******************************************************************************
  */

#include "app_state.h"

#include <stddef.h>

static bool AppState_IsKnownEvent(AppStateEvent event)
{
  switch (event)
  {
    case APP_STATE_EVENT_INIT_COMPLETE:
    case APP_STATE_EVENT_START:
    case APP_STATE_EVENT_STOP:
    case APP_STATE_EVENT_FAULT_DETECTED:
    case APP_STATE_EVENT_FAULT_CLEARED:
    case APP_STATE_EVENT_FAULT_RESET:
      return true;

    default:
      return false;
  }
}

AppStateError AppState_Init(AppStateContext *context)
{
  if (context == NULL)
  {
    return APP_STATE_ERR_NULL_CONTEXT;
  }

  context->state = APP_STATE_INIT;
  context->fault_clear_confirmed = false;

  return APP_STATE_OK;
}

AppStateError AppState_HandleEvent(AppStateContext *context, AppStateEvent event)
{
  if (context == NULL)
  {
    return APP_STATE_ERR_NULL_CONTEXT;
  }

  if (!AppState_IsKnownEvent(event))
  {
    return APP_STATE_ERR_INVALID_EVENT;
  }

  switch (context->state)
  {
    case APP_STATE_INIT:
      if (event == APP_STATE_EVENT_INIT_COMPLETE)
      {
        context->state = APP_STATE_IDLE;
        return APP_STATE_OK;
      }
      return APP_STATE_ERR_INVALID_TRANSITION;

    case APP_STATE_IDLE:
      if (event == APP_STATE_EVENT_START)
      {
        context->state = APP_STATE_RUNNING;
        return APP_STATE_OK;
      }
      if (event == APP_STATE_EVENT_FAULT_DETECTED)
      {
        context->state = APP_STATE_FAULT;
        context->fault_clear_confirmed = false;
        return APP_STATE_OK;
      }
      return APP_STATE_ERR_INVALID_TRANSITION;

    case APP_STATE_RUNNING:
      if (event == APP_STATE_EVENT_STOP)
      {
        context->state = APP_STATE_IDLE;
        return APP_STATE_OK;
      }
      if (event == APP_STATE_EVENT_FAULT_DETECTED)
      {
        context->state = APP_STATE_FAULT;
        context->fault_clear_confirmed = false;
        return APP_STATE_OK;
      }
      return APP_STATE_ERR_INVALID_TRANSITION;

    case APP_STATE_FAULT:
      if (event == APP_STATE_EVENT_FAULT_DETECTED)
      {
        return APP_STATE_OK;
      }
      if (event == APP_STATE_EVENT_FAULT_CLEARED)
      {
        context->fault_clear_confirmed = true;
        return APP_STATE_OK;
      }
      if (event == APP_STATE_EVENT_FAULT_RESET)
      {
        if (!context->fault_clear_confirmed)
        {
          return APP_STATE_ERR_FAULT_NOT_CLEARED;
        }

        context->fault_clear_confirmed = false;
        context->state = APP_STATE_IDLE;
        return APP_STATE_OK;
      }
      return APP_STATE_ERR_INVALID_TRANSITION;

    default:
      return APP_STATE_ERR_INVALID_TRANSITION;
  }
}

const char *AppState_GetStateName(AppState state)
{
  switch (state)
  {
    case APP_STATE_INIT:
      return "INIT";

    case APP_STATE_IDLE:
      return "IDLE";

    case APP_STATE_RUNNING:
      return "RUNNING";

    case APP_STATE_FAULT:
      return "FAULT";

    default:
      return "UNKNOWN";
  }
}
