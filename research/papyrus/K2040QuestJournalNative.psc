Scriptname K2040QuestJournalNative Native Hidden
; Source-only interface design. The production plugin DOES NOT register this.
; No helper record or compiled PEX is distributed.

Bool Function IsEnabled() Global Native
; [positive ticket, signed 32-bit FormID bits, desired tracking 0 or 1].
; None/empty means no request. Every claim must be consumed at most once.
Int[] Function Claim() Global Native
Bool Function IsCurrent(Int aiTicket) Global Native
Bool Function Complete(Int aiTicket, Bool abObservedTracked) Global Native
Bool Function Cancel(Int aiTicket) Global Native
