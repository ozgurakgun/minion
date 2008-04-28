

// to be included when TriggerMem is defined.

#ifdef MIXEDTRIGGERS
inline void DynamicTrigger::sleepDynamicTriggerBT(StateObj * stateObj)
  {
      sleepWatchTrigger(stateObj);
      getTriggerMem(stateObj).add_bt_record(this, queue);
      queue=NULL;
  }
#endif
