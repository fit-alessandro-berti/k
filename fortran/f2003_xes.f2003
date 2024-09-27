program xes_processor
  implicit none
  ! Declare types
  type Event
     character(len=:), allocatable :: activity
  end type Event

  type Trace
     type(Event), allocatable :: events(:)
  end type Trace

  type Log
     type(Trace), allocatable :: traces(:)
  end type Log

  ! Declare variables
  type(Log) :: logData

  ! Main program variables
  integer :: argc
  character(len=256) :: inputFile, outputFile

  ! Get the number of command-line arguments
  call get_command_argument(0, inputFile)
  argc = command_argument_count()

  if (argc /= 2) then
     print *, 'Usage: ./xes_processor input.xes output.xes'
     stop
  end if

  ! Get input and output file names
  call get_command_argument(1, inputFile)
  call get_command_argument(2, outputFile)

  ! Import XES file
  call importXES(inputFile, logData)

  ! Export XES file
  call exportXES(outputFile, logData)

contains

  subroutine importXES(filename, logData)
    character(len=*), intent(in) :: filename
    type(Log), intent(out) :: logData

    integer :: unit, ios
    character(len=1000) :: line
    logical :: inTrace, inEvent
    type(Trace) :: currentTrace
    type(Event) :: currentEvent

    integer :: traceCount, eventCount

    ! Initialize variables
    inTrace = .false.
    inEvent = .false.
    traceCount = 0
    eventCount = 0
    if (allocated(logData%traces)) deallocate(logData%traces)
    if (allocated(currentTrace%events)) deallocate(currentTrace%events)

    ! Open the file
    open(newunit=unit, file=filename, status='old', action='read', iostat=ios)
    if (ios /= 0) then
       print *, 'Error opening file: ', filename
       stop
    end if

    ! Loop through the file
    do
      read(unit, '(A)', iostat=ios) line
      if (ios /= 0) exit

      ! Trim leading spaces
      line = adjustl(line)

      ! Check for trace start
      if (index(line, '<trace') > 0) then
         inTrace = .true.
         eventCount = 0
         if (allocated(currentTrace%events)) deallocate(currentTrace%events)
         cycle
      end if

      ! Check for trace end
      if (index(line, '</trace') > 0) then
         inTrace = .false.
         traceCount = traceCount + 1
         if (.not. allocated(logData%traces)) then
            allocate(logData%traces(1))
         else
            call resizeTraces(logData%traces, traceCount)
         end if
         logData%traces(traceCount) = currentTrace
         if (allocated(currentTrace%events)) deallocate(currentTrace%events)
         cycle
      end if

      if (inTrace) then
         ! Check for event start
         if (index(line, '<event') > 0) then
            inEvent = .true.
            currentEvent%activity = ''
            cycle
         end if

         ! Check for event end
         if (index(line, '</event') > 0) then
            inEvent = .false.
            eventCount = eventCount + 1
            if (.not. allocated(currentTrace%events)) then
               allocate(currentTrace%events(1))
            else
               call resizeEvents(currentTrace%events, eventCount)
            end if
            currentTrace%events(eventCount) = currentEvent
            cycle
         end if

         if (inEvent) then
            ! Check for concept:name attribute
            if (index(line, 'key="concept:name"') > 0) then
               call extractActivity(line, currentEvent%activity)
            end if
         end if
      end if
    end do

    close(unit)
  end subroutine importXES

  subroutine extractActivity(line, activity)
    character(len=*), intent(in) :: line
    character(len=:), allocatable, intent(out) :: activity
    integer :: startPos, endPos

    ! Find the value attribute
    startPos = index(line, 'value="') + len('value="')
    if (startPos > len('value="')) then
       endPos = index(line(startPos:), '"') + startPos - 2
       activity = line(startPos:endPos)
    else
       activity = ''
    end if
  end subroutine extractActivity

  subroutine exportXES(filename, logData)
    character(len=*), intent(in) :: filename
    type(Log), intent(in) :: logData

    integer :: unit, ios, i, j

    ! Open the file for writing
    open(newunit=unit, file=filename, status='replace', action='write', iostat=ios)
    if (ios /= 0) then
       print *, 'Error opening file for writing: ', filename
       stop
    end if

    ! Write XML header
    write(unit, '(A)') '<?xml version="1.0" encoding="UTF-8"?>'
    write(unit, '(A)') '<log>'

    ! Loop over traces
    do i = 1, size(logData%traces)
       write(unit, '(A)') '  <trace>'
       ! Loop over events
       do j = 1, size(logData%traces(i)%events)
          write(unit, '(A)') '    <event>'
          write(unit, '(A)', advance='no') '      <string key="concept:name" value="'
          write(unit, '(A)', advance='no') trim(logData%traces(i)%events(j)%activity)
          write(unit, '(A)') '"/>'
          write(unit, '(A)') '    </event>'
       end do
       write(unit, '(A)') '  </trace>'
    end do

    write(unit, '(A)') '</log>'
    close(unit)
  end subroutine exportXES

  subroutine resizeTraces(traces, newSize)
    type(Trace), allocatable, intent(inout) :: traces(:)
    integer, intent(in) :: newSize
    type(Trace), allocatable :: temp(:)
    integer :: oldSize

    if (allocated(traces)) then
        oldSize = size(traces)
    else
        oldSize = 0
    end if

    allocate(temp(newSize))
    if (oldSize > 0) then
        temp(1:oldSize) = traces
    end if
    deallocate(traces)
    traces = temp
  end subroutine resizeTraces

  subroutine resizeEvents(events, newSize)
    type(Event), allocatable, intent(inout) :: events(:)
    integer, intent(in) :: newSize
    type(Event), allocatable :: temp(:)
    integer :: oldSize

    if (allocated(events)) then
        oldSize = size(events)
    else
        oldSize = 0
    end if

    allocate(temp(newSize))
    if (oldSize > 0) then
        temp(1:oldSize) = events
    end if
    deallocate(events)
    events = temp
  end subroutine resizeEvents

end program xes_processor