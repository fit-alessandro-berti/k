program ocel_import_export
  implicit none

  ! Declare variables and parameters
  ! Maximum sizes for arrays
  integer, parameter :: max_events = 1000
  integer, parameter :: max_objects = 1000
  integer, parameter :: max_event_types = 100
  integer, parameter :: max_object_types = 100
  integer, parameter :: max_attributes = 100
  integer, parameter :: max_relationships = 100
  integer, parameter :: max_string_length = 256

  ! Type definitions
  type :: Attribute
    character(len=max_string_length) :: name
    character(len=max_string_length) :: value
    character(len=max_string_length) :: time
    character(len=max_string_length) :: type
  end type Attribute

  type :: Relationship
    character(len=max_string_length) :: objectId
    character(len=max_string_length) :: qualifier
  end type Relationship

  type :: Event
    character(len=max_string_length) :: id
    character(len=max_string_length) :: type_name
    character(len=max_string_length) :: time
    integer :: num_attributes
    type(Attribute), dimension(:), allocatable :: attributes
    integer :: num_relationships
    type(Relationship), dimension(:), allocatable :: relationships
  end type Event

  type :: Object
    character(len=max_string_length) :: id
    character(len=max_string_length) :: type_name
    integer :: num_attributes
    type(Attribute), dimension(:), allocatable :: attributes
    integer :: num_relationships
    type(Relationship), dimension(:), allocatable :: relationships
  end type Object

  type :: EventType
    character(len=max_string_length) :: name
    integer :: num_attributes
    type(Attribute), dimension(:), allocatable :: attributes
  end type EventType

  type :: ObjectType
    character(len=max_string_length) :: name
    integer :: num_attributes
    type(Attribute), dimension(:), allocatable :: attributes
  end type ObjectType

  ! Data structures
  integer :: num_events = 0
  type(Event), dimension(:), allocatable :: events

  integer :: num_objects = 0
  type(Object), dimension(:), allocatable :: objects

  integer :: num_event_types = 0
  type(EventType), dimension(:), allocatable :: event_types

  integer :: num_object_types = 0
  type(ObjectType), dimension(:), allocatable :: object_types

  ! Variables
  integer :: argc, status
  character(len=max_string_length) :: input_filename, output_filename

  ! Main program execution

  ! Get the command-line argument count
  ! Get the number of command-line arguments
  call get_command_argument(0, input_filename)
  argc = command_argument_count()

  if (argc /= 2) then
     print *, 'Usage: ocel_import_export <input_json_file> <output_json_file>'
     stop
  end if

  ! Get input and output file names
  call get_command_argument(1, input_filename)
  call get_command_argument(2, output_filename)

  ! Call import and export subroutines
  call import_ocel(trim(input_filename))
  call export_ocel(trim(output_filename))

  print *, 'OCEL JSON import and export completed successfully.'

contains

  subroutine import_ocel(filename)
    implicit none
    character(len=*), intent(in) :: filename
    ! Variables for file reading
    character(len=1000) :: line
    integer :: ios, unit
    integer :: i
    logical :: done
    integer :: pos

    ! Open the JSON file for reading
    open(newunit=unit, file=filename, status='old', action='read', iostat=ios)
    if (ios /= 0) then
      print *, 'Error opening file ', filename
      stop
    end if

    ! Allocate arrays
    allocate(events(max_events))
    allocate(objects(max_objects))
    allocate(event_types(max_event_types))
    allocate(object_types(max_object_types))

    ! Initialize counts
    num_events = 0
    num_objects = 0
    num_event_types = 0
    num_object_types = 0

    ! Read and parse the file
    done = .false.
    do while (.not. done)
      read(unit, '(A)', iostat=ios) line
      if (ios /= 0) exit

      ! Remove leading spaces
      line = adjustl(line)

      if (line == '') cycle

      ! Determine which section we are in
      if (index(line, '"objectTypes"') > 0) then
        call parse_objectTypes(unit)
      else if (index(line, '"eventTypes"') > 0) then
        call parse_eventTypes(unit)
      else if (index(line, '"objects"') > 0) then
        call parse_objects(unit)
      else if (index(line, '"events"') > 0) then
        call parse_events(unit)
      end if
    end do

    close(unit)
  end subroutine import_ocel

  subroutine parse_objectTypes(unit)
    implicit none
    integer, intent(in) :: unit
    character(len=1000) :: line
    integer :: ios
    integer :: i, j
    character(len=max_string_length) :: key, value
    logical :: isEnd

    isEnd = .false.
    do while (.not. isEnd)
      read(unit, '(A)', iostat=ios) line
      if (ios /= 0) exit

      line = adjustl(line)
      if (line == '') cycle

      if (trim(line(1:1)) == ']') then
        isEnd = .true.
        exit
      end if

      if (index(line, '{') > 0) then
        num_object_types = num_object_types + 1
        object_types(num_object_types)%num_attributes = 0
        allocate(object_types(num_object_types)%attributes(max_attributes))
        ! Parse object type
        do
          read(unit, '(A)', iostat=ios) line
          if (ios /= 0) exit
          line = adjustl(line)
          if (index(line, '"name"') > 0) then
            call parse_key_value(line, key, value)
            object_types(num_object_types)%name = value
          else if (index(line, '"attributes"') > 0) then
            call parse_attributes(unit, object_types(num_object_types)%attributes, &
                                  object_types(num_object_types)%num_attributes)
          else if (index(line, '}') > 0) then
            exit
          end if
        end do
      end if
    end do
  end subroutine parse_objectTypes

  subroutine parse_eventTypes(unit)
    implicit none
    integer, intent(in) :: unit
    character(len=1000) :: line
    integer :: ios
    integer :: i, j
    character(len=max_string_length) :: key, value
    logical :: isEnd

    isEnd = .false.
    do while (.not. isEnd)
      read(unit, '(A)', iostat=ios) line
      if (ios /= 0) exit

      line = adjustl(line)
      if (line == '') cycle

      if (trim(line(1:1)) == ']') then
        isEnd = .true.
        exit
      end if

      if (index(line, '{') > 0) then
        num_event_types = num_event_types + 1
        event_types(num_event_types)%num_attributes = 0
        allocate(event_types(num_event_types)%attributes(max_attributes))
        ! Parse event type
        do
          read(unit, '(A)', iostat=ios) line
          if (ios /= 0) exit
          line = adjustl(line)
          if (index(line, '"name"') > 0) then
            call parse_key_value(line, key, value)
            event_types(num_event_types)%name = value
          else if (index(line, '"attributes"') > 0) then
            call parse_attributes(unit, event_types(num_event_types)%attributes, &
                                  event_types(num_event_types)%num_attributes)
          else if (index(line, '}') > 0) then
            exit
          end if
        end do
      end if
    end do
  end subroutine parse_eventTypes

  subroutine parse_objects(unit)
    implicit none
    integer, intent(in) :: unit
    character(len=1000) :: line
    integer :: ios
    integer :: i, j
    character(len=max_string_length) :: key, value
    logical :: isEnd

    isEnd = .false.
    do while (.not. isEnd)
      read(unit, '(A)', iostat=ios) line
      if (ios /= 0) exit

      line = adjustl(line)
      if (line == '') cycle

      if (trim(line(1:1)) == ']') then
        isEnd = .true.
        exit
      end if

      if (index(line, '{') > 0) then
        num_objects = num_objects + 1
        objects(num_objects)%num_attributes = 0
        objects(num_objects)%num_relationships = 0
        allocate(objects(num_objects)%attributes(max_attributes))
        allocate(objects(num_objects)%relationships(max_relationships))
        ! Parse object
        do
          read(unit, '(A)', iostat=ios) line
          if (ios /= 0) exit
          line = adjustl(line)
          if (index(line, '"id"') > 0) then
            call parse_key_value(line, key, value)
            objects(num_objects)%id = value
          else if (index(line, '"type"') > 0) then
            call parse_key_value(line, key, value)
            objects(num_objects)%type_name = value
          else if (index(line, '"attributes"') > 0) then
            call parse_attributes(unit, objects(num_objects)%attributes, &
                                  objects(num_objects)%num_attributes)
          else if (index(line, '"relationships"') > 0) then
            call parse_relationships(unit, objects(num_objects)%relationships, &
                                     objects(num_objects)%num_relationships)
          else if (index(line, '}') > 0) then
            exit
          end if
        end do
      end if
    end do
  end subroutine parse_objects

  subroutine parse_events(unit)
    implicit none
    integer, intent(in) :: unit
    character(len=1000) :: line
    integer :: ios
    integer :: i, j
    character(len=max_string_length) :: key, value
    logical :: isEnd

    isEnd = .false.
    do while (.not. isEnd)
      read(unit, '(A)', iostat=ios) line
      if (ios /= 0) exit

      line = adjustl(line)
      if (line == '') cycle

      if (trim(line(1:1)) == ']') then
        isEnd = .true.
        exit
      end if

      if (index(line, '{') > 0) then
        num_events = num_events + 1
        events(num_events)%num_attributes = 0
        events(num_events)%num_relationships = 0
        allocate(events(num_events)%attributes(max_attributes))
        allocate(events(num_events)%relationships(max_relationships))
        ! Parse event
        do
          read(unit, '(A)', iostat=ios) line
          if (ios /= 0) exit
          line = adjustl(line)
          if (index(line, '"id"') > 0) then
            call parse_key_value(line, key, value)
            events(num_events)%id = value
          else if (index(line, '"type"') > 0) then
            call parse_key_value(line, key, value)
            events(num_events)%type_name = value
          else if (index(line, '"time"') > 0) then
            call parse_key_value(line, key, value)
            events(num_events)%time = value
          else if (index(line, '"attributes"') > 0) then
            call parse_attributes(unit, events(num_events)%attributes, &
                                  events(num_events)%num_attributes)
          else if (index(line, '"relationships"') > 0) then
            call parse_relationships(unit, events(num_events)%relationships, &
                                     events(num_events)%num_relationships)
          else if (index(line, '}') > 0) then
            exit
          end if
        end do
      end if
    end do
  end subroutine parse_events

  subroutine parse_attributes(unit, attributes, num_attributes)
    implicit none
    integer, intent(in) :: unit
    type(Attribute), dimension(:), intent(in out) :: attributes
    integer, intent(in out) :: num_attributes
    character(len=1000) :: line
    integer :: ios
    character(len=max_string_length) :: key, value
    logical :: isEnd, inAttribute
    integer :: length_line

    isEnd = .false.
    inAttribute = .false.
    do while (.not. isEnd)
      read(unit, '(A)', iostat=ios) line
      if (ios /= 0) exit

      line = adjustl(line)
      length_line = len_trim(line)
      if (length_line == 0) cycle

      if (trim(line(1:1)) == ']') then
        isEnd = .true.
        exit
      end if

      if (index(line, '{') > 0) then
        num_attributes = num_attributes + 1
        inAttribute = .true.
        ! Parse attribute
        do
          read(unit, '(A)', iostat=ios) line
          if (ios /= 0) exit
          line = adjustl(line)
          if (index(line, '}') > 0) then
            inAttribute = .false.
            exit
          else
            call parse_key_value(line, key, value)
            if (key == 'name') then
              attributes(num_attributes)%name = value
            else if (key == 'value') then
              attributes(num_attributes)%value = value
            else if (key == 'type') then
              attributes(num_attributes)%type = value
            else if (key == 'time') then
              attributes(num_attributes)%time = value
            end if
          end if
        end do
      end if
    end do
  end subroutine parse_attributes

  subroutine parse_relationships(unit, relationships, num_relationships)
    implicit none
    integer, intent(in) :: unit
    type(Relationship), dimension(:), intent(in out) :: relationships
    integer, intent(in out) :: num_relationships
    character(len=1000) :: line
    integer :: ios
    character(len=max_string_length) :: key, value
    logical :: isEnd, inRelationship

    isEnd = .false.
    inRelationship = .false.
    do while (.not. isEnd)
      read(unit, '(A)', iostat=ios) line
      if (ios /= 0) exit

      line = adjustl(line)
      if (line == '') cycle

      if (trim(line(1:1)) == ']') then
        isEnd = .true.
        exit
      end if

      if (index(line, '{') > 0) then
        num_relationships = num_relationships + 1
        inRelationship = .true.
        ! Parse relationship
        do
          read(unit, '(A)', iostat=ios) line
          if (ios /= 0) exit
          line = adjustl(line)
          if (index(line, '}') > 0) then
            inRelationship = .false.
            exit
          else
            call parse_key_value(line, key, value)
            if (key == 'objectId') then
              relationships(num_relationships)%objectId = value
            else if (key == 'qualifier') then
              relationships(num_relationships)%qualifier = value
            end if
          end if
        end do
      end if
    end do
  end subroutine parse_relationships

  subroutine parse_key_value(line, key, value)
    implicit none
    character(len=*), intent(in) :: line
    character(len=*), intent(out) :: key, value
    integer :: pos_colon, pos_quote1, pos_quote2, pos_quote3, pos_quote4

    ! Find the positions of the quotes and colon
    pos_colon = index(line, ':')
    pos_quote1 = index(line, '"')
    pos_quote2 = index(line(pos_quote1+1:), '"') + pos_quote1
    pos_quote3 = index(line(pos_colon+1:), '"') + pos_colon
    pos_quote4 = index(line(pos_quote3+1:), '"') + pos_quote3

    key = line(pos_quote1+1:pos_quote2-1)
    value = line(pos_quote3+1:pos_quote4-1)
  end subroutine parse_key_value

  subroutine export_ocel(filename)
    implicit none
    character(len=*), intent(in) :: filename
    integer :: unit, ios, i, j
    ! Open the JSON file for writing
    open(newunit=unit, file=filename, status='replace', action='write', iostat=ios)
    if (ios /= 0) then
      print *, 'Error opening file ', filename
      stop
    end if

    write(unit, '(A)') '{'

    ! Export objectTypes
    write(unit, '(A)') '  "objectTypes": ['
    do i = 1, num_object_types
      write(unit, '(A)', advance='no') '    {'
      write(unit, '(A)', advance='no') '"name": "'//trim(object_types(i)%name)//'",'
      write(unit, '(A)', advance='no') '"attributes": ['
      do j = 1, object_types(i)%num_attributes
        write(unit, '(A)', advance='no') '{'
        write(unit, '(A)', advance='no') '"name": "'//trim(object_types(i)%attributes(j)%name)//'",'
        write(unit, '(A)', advance='no') '"type": "'//trim(object_types(i)%attributes(j)%type)//'"'
        write(unit, '(A)', advance='no') '}'
        if (j < object_types(i)%num_attributes) then
          write(unit, '(A)', advance='no') ','
        end if
      end do
      write(unit, '(A)', advance='no') ']}'
      if (i < num_object_types) then
        write(unit, '(A)', advance='no') ','
      end if
    end do
    write(unit, '(A)') '  ],'

    ! Export eventTypes
    write(unit, '(A)') '  "eventTypes": ['
    do i = 1, num_event_types
      write(unit, '(A)', advance='no') '    {'
      write(unit, '(A)', advance='no') '"name": "'//trim(event_types(i)%name)//'",'
      write(unit, '(A)', advance='no') '"attributes": ['
      do j = 1, event_types(i)%num_attributes
        write(unit, '(A)', advance='no') '{'
        write(unit, '(A)', advance='no') '"name": "'//trim(event_types(i)%attributes(j)%name)//'",'
        write(unit, '(A)', advance='no') '"type": "'//trim(event_types(i)%attributes(j)%type)//'"'
        write(unit, '(A)', advance='no') '}'
        if (j < event_types(i)%num_attributes) then
          write(unit, '(A)', advance='no') ','
        end if
      end do
      write(unit, '(A)', advance='no') ']}'
      if (i < num_event_types) then
        write(unit, '(A)', advance='no') ','
      end if
    end do
    write(unit, '(A)') '  ],'

    ! Export objects
    write(unit, '(A)') '  "objects": ['
    do i = 1, num_objects
      write(unit, '(A)', advance='no') '    {'
      write(unit, '(A)', advance='no') '"id": "'//trim(objects(i)%id)//'",'
      write(unit, '(A)', advance='no') '"type": "'//trim(objects(i)%type_name)//'",'
      write(unit, '(A)', advance='no') '"attributes": ['
      do j = 1, objects(i)%num_attributes
        write(unit, '(A)', advance='no') '{'
        write(unit, '(A)', advance='no') '"name": "'//trim(objects(i)%attributes(j)%name)//'",'
        write(unit, '(A)', advance='no') '"time": "'//trim(objects(i)%attributes(j)%time)//'",'
        write(unit, '(A)', advance='no') '"value": "'//trim(objects(i)%attributes(j)%value)//'"'
        write(unit, '(A)', advance='no') '}'
        if (j < objects(i)%num_attributes) then
          write(unit, '(A)', advance='no') ','
        end if
      end do
      write(unit, '(A)', advance='no') '],'
      write(unit, '(A)', advance='no') '"relationships": ['
      do j = 1, objects(i)%num_relationships
        write(unit, '(A)', advance='no') '{'
        write(unit, '(A)', advance='no') '"objectId": "'//trim(objects(i)%relationships(j)%objectId)//'",'
        write(unit, '(A)', advance='no') '"qualifier": "'//trim(objects(i)%relationships(j)%qualifier)//'"'
        write(unit, '(A)', advance='no') '}'
        if (j < objects(i)%num_relationships) then
          write(unit, '(A)', advance='no') ','
        end if
      end do
      write(unit, '(A)', advance='no') ']}'
      if (i < num_objects) then
        write(unit, '(A)', advance='no') ','
      end if
    end do
    write(unit, '(A)') '  ],'

    ! Export events
    write(unit, '(A)') '  "events": ['
    do i = 1, num_events
      write(unit, '(A)', advance='no') '    {'
      write(unit, '(A)', advance='no') '"id": "'//trim(events(i)%id)//'",'
      write(unit, '(A)', advance='no') '"type": "'//trim(events(i)%type_name)//'",'
      write(unit, '(A)', advance='no') '"time": "'//trim(events(i)%time)//'",'
      write(unit, '(A)', advance='no') '"attributes": ['
      do j = 1, events(i)%num_attributes
        write(unit, '(A)', advance='no') '{'
        write(unit, '(A)', advance='no') '"name": "'//trim(events(i)%attributes(j)%name)//'",'
        write(unit, '(A)', advance='no') '"value": "'//trim(events(i)%attributes(j)%value)//'"'
        write(unit, '(A)', advance='no') '}'
        if (j < events(i)%num_attributes) then
          write(unit, '(A)', advance='no') ','
        end if
      end do
      write(unit, '(A)', advance='no') '],'
      write(unit, '(A)', advance='no') '"relationships": ['
      do j = 1, events(i)%num_relationships
        write(unit, '(A)', advance='no') '{'
        write(unit, '(A)', advance='no') '"objectId": "'//trim(events(i)%relationships(j)%objectId)//'",'
        write(unit, '(A)', advance='no') '"qualifier": "'//trim(events(i)%relationships(j)%qualifier)//'"'
        write(unit, '(A)', advance='no') '}'
        if (j < events(i)%num_relationships) then
          write(unit, '(A)', advance='no') ','
        end if
      end do
      write(unit, '(A)', advance='no') ']}'
      if (i < num_events) then
        write(unit, '(A)', advance='no') ','
      end if
    end do
    write(unit, '(A)') '  ]'

    write(unit, '(A)') '}'
    close(unit)
  end subroutine export_ocel

end program ocel_import_export