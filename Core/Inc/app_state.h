/**
  ******************************************************************************
  * @file    app_state.h
  * @brief   Hardware-independent equipment state machine.
  ******************************************************************************
  */

#ifndef APP_STATE_H
#define APP_STATE_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
  APP_STATE_INIT = 0,
  APP_STATE_IDLE,
  APP_STATE_RUNNING,
  APP_STATE_FAULT
} AppState;

typedef enum
{
  APP_STATE_EVENT_INIT_COMPLETE = 0,
  APP_STATE_EVENT_START,
  APP_STATE_EVENT_STOP,
  APP_STATE_EVENT_FAULT_DETECTED,
  APP_STATE_EVENT_FAULT_CLEARED,
  APP_STATE_EVENT_FAULT_RESET,
  APP_STATE_EVENT_COUNT
} AppStateEvent;

typedef enum
{
  APP_STATE_OK = 0,
  APP_STATE_ERR_NULL_CONTEXT,
  APP_STATE_ERR_INVALID_EVENT,
  APP_STATE_ERR_INVALID_TRANSITION,
  APP_STATE_ERR_FAULT_NOT_CLEARED
} AppStateError;

typedef struct
{
  AppState state;
  bool fault_clear_confirmed;
} AppStateContext;

AppStateError AppState_Init(AppStateContext *context);
AppStateError AppState_HandleEvent(AppStateContext *context, AppStateEvent event);
const char *AppState_GetStateName(AppState state);

#ifdef __cplusplus
}
#endif

#endif /* APP_STATE_H */
