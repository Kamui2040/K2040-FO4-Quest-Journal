Scriptname K2040QuestJournalWorker extends Quest
; PROTOTYPE ONLY. Do not compile/package, bind to an ESP/ESL or start a quest
; until the runtime bridge, Papyrus compiler and save isolation are verified.
; IsEnabled must remain false until explicitly authorized and tested.

Int Property PollTimerId = 27 AutoReadOnly Hidden
Float Property PollInterval = 1.0 AutoReadOnly Hidden

Event OnQuestInit()
    If K2040QuestJournalNative.IsEnabled()
        StartTimer(PollInterval, PollTimerId)
    EndIf
EndEvent

Event OnTimer(Int aiTimerID)
    If aiTimerID != PollTimerId || !K2040QuestJournalNative.IsEnabled()
        Return
    EndIf

    Int[] request = K2040QuestJournalNative.Claim()
    If request && request.Length == 3
        Int ticket = request[0]
        Int formID = request[1]
        Int desired = request[2]
        If ticket > 0 && (desired == 0 || desired == 1)
            Quest target = Game.GetForm(formID) as Quest
            If target && target.IsRunning() && !target.IsCompleted()
                If K2040QuestJournalNative.IsCurrent(ticket)
                    ; Only the engine-owned script method may eventually write.
                    target.SetActive(desired == 1)
                    K2040QuestJournalNative.Complete(ticket, target.IsActive())
                Else
                    K2040QuestJournalNative.Cancel(ticket)
                EndIf
            Else
                K2040QuestJournalNative.Cancel(ticket)
            EndIf
        EndIf
    EndIf

    If K2040QuestJournalNative.IsEnabled()
        StartTimer(PollInterval, PollTimerId)
    EndIf
EndEvent
