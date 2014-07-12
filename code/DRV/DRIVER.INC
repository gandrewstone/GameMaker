; Copyright 1994 G. Andrew Stone                                05/26/94

; Driver.inc   ASM Description of the driver data structure.
;              the VideoMode struc must correspond to the C++ VideoMode Class

; NOT PUBLIC DOMAIN!
;------------

STRUC      DriverHeader
           ID           db        'GM',0
           Object       db        'Video Driver',0
           DrvrVersion  dw        110
           Date         db        '00-00-00',0 ;??Date,0
           Xtra         db        0
           DriverType   db        32 DUP(0)
           NumModes     db        1
ENDS       DriverHeader



STRUC      VideoMode
           Length     dw         320
           Width      dw         200
           LineSep    dw         320
           ColDepth   db         8
           Address    dd         0A0000000h
           VPageOff   dd         0
           BIOSMode   dw         013h

           SetMode    dd         0
           Clear      dd         0
           Point      dd         0
           GetCol     dd         0
           Rep1Col    dd         0

           Rep1ColRev dd         0
           RepCols    dd         0
           RepColsRev dd         0
           GetCols    dd         0
           GetColsRev dd         0

           Line       dd         0
           Box        dd         0
           BoxFill    dd         0
           DrawBlock  dd         0
           BCurve     dd         0

           Spline     dd         0
           SetViewPos   dd        0
           SetWritePage dd        0
           SetAllPal    dd        0    ;; Will be changed to a range soon.
           GetAllPal    dd        0

           SetAllPalTo  dd        0
           SetPal       dd        0
ENDS       VideoMode

