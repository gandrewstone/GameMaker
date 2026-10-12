# Mouse step helpers for key scripts (dot-source with keyscript.ps1). The cursor is first sent to the upper left corner (the
# first movement event of a DOS run is unreliable: see KEYS.C and README.md), and the helpers then track its position, so the
# scripts can name absolute screen positions (320x200 units). They assume the editor lets the cursor reach (0,0).
$script:mx = 0; $script:my = 0; $script:mb = 0

function Mouse-Start { $script:mx = 0; $script:my = 0; $script:mb = 0; @('mouse:4,4,0', 'mouse:-400,-400,0') }

function Mouse-Move([int]$x, [int]$y) {
  $dx = $x - $script:mx; $dy = $y - $script:my
  $script:mx = $x; $script:my = $y
  "mouse:$dx,$dy,$($script:mb)"
}
function Mouse-Click([int]$x, [int]$y, [int]$button = 1) {
  Mouse-Move $x $y
  $script:mb = $button; "mouse:0,0,$button"
  $script:mb = 0; 'mouse:0,0,0'
}
# press at (x1,y1), move to (x2,y2) in steps of at most <step> units with the button held, release
function Mouse-Drag([int]$x1, [int]$y1, [int]$x2, [int]$y2, [int]$step = 4, [int]$button = 1) {
  Mouse-Move $x1 $y1
  $script:mb = $button; "mouse:0,0,$button"
  $n = [Math]::Max([Math]::Abs($x2 - $x1), [Math]::Abs($y2 - $y1)); $k = [Math]::Max(1, [int][Math]::Ceiling($n / $step))
  for ($i = 1; $i -le $k; $i++) { Mouse-Move ([int][Math]::Round($x1 + ($x2 - $x1) * $i / $k)) ([int][Math]::Round($y1 + ($y2 - $y1) * $i / $k)) }
  $script:mb = 0; 'mouse:0,0,0'
}
