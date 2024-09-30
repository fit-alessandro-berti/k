require 'rexml/document'
include REXML

if ARGV.length != 2
  puts "Usage: ruby #{__FILE__} input_file.xml output_file.xml"
  exit
end

input_file = ARGV[0]
output_file = ARGV[1]

# Data structures
class ObjectType
  attr_accessor :name, :attributes
  def initialize(name)
    @name = name
    @attributes = [] # List of Attribute(name, type)
  end
end

class EventType
  attr_accessor :name, :attributes
  def initialize(name)
    @name = name
    @attributes = [] # List of Attribute(name, type)
  end
end

class Attribute
  attr_accessor :name, :type
  def initialize(name, type)
    @name = name
    @type = type
  end
end

class OCELObject
  attr_accessor :id, :type, :attributes, :relationships
  def initialize(id, type)
    @id = id
    @type = type
    @attributes = [] # List of ObjectAttribute(name, time, value)
    @relationships = [] # List of Relationship(object_id, qualifier)
  end
end

class ObjectAttribute
  attr_accessor :name, :time, :value
  def initialize(name, time, value)
    @name = name
    @time = time
    @value = value
  end
end

class Event
  attr_accessor :id, :type, :time, :attributes, :relationships
  def initialize(id, type, time)
    @id = id
    @type = type
    @time = time
    @attributes = [] # List of EventAttribute(name, value)
    @relationships = [] # List of Relationship(object_id, qualifier)
  end
end

class EventAttribute
  attr_accessor :name, :value
  def initialize(name, value)
    @name = name
    @value = value
  end
end

class Relationship
  attr_accessor :object_id, :qualifier
  def initialize(object_id, qualifier)
    @object_id = object_id
    @qualifier = qualifier
  end
end

# Parse the input file
begin
  file = File.open(input_file)
  doc = Document.new(file)
  file.close
rescue Exception => e
  puts "Error: #{e}"
  exit
end

# Containers
object_types = []
event_types = []
objects = []
events = []

# Parse the <log> element
log = doc.root

# Parse object-types
log.elements.each('object-types/object-type') do |ot_elem|
  ot_name = ot_elem.attributes['name']
  object_type = ObjectType.new(ot_name)
  ot_elem.elements.each('attributes/attribute') do |attr_elem|
    attr_name = attr_elem.attributes['name']
    attr_type = attr_elem.attributes['type']
    attribute = Attribute.new(attr_name, attr_type)
    object_type.attributes << attribute
  end
  object_types << object_type
end

# Parse event-types
log.elements.each('event-types/event-type') do |et_elem|
  et_name = et_elem.attributes['name']
  event_type = EventType.new(et_name)
  et_elem.elements.each('attributes/attribute') do |attr_elem|
    attr_name = attr_elem.attributes['name']
    attr_type = attr_elem.attributes['type']
    attribute = Attribute.new(attr_name, attr_type)
    event_type.attributes << attribute
  end
  event_types << event_type
end

# Parse objects
log.elements.each('objects/object') do |obj_elem|
  obj_id = obj_elem.attributes['id']
  obj_type = obj_elem.attributes['type']
  object = OCELObject.new(obj_id, obj_type)
  # Parse attributes
  obj_elem.elements.each('attributes/attribute') do |attr_elem|
    attr_name = attr_elem.attributes['name']
    attr_time = attr_elem.attributes['time']
    attr_value = attr_elem.text
    object_attribute = ObjectAttribute.new(attr_name, attr_time, attr_value)
    object.attributes << object_attribute
  end
  # Parse relationships
  obj_elem.elements.each('objects/relationship') do |rel_elem|
    rel_object_id = rel_elem.attributes['object-id']
    rel_qualifier = rel_elem.attributes['qualifier']
    relationship = Relationship.new(rel_object_id, rel_qualifier)
    object.relationships << relationship
  end
  objects << object
end

# Parse events
log.elements.each('events/event') do |event_elem|
  event_id = event_elem.attributes['id']
  event_type = event_elem.attributes['type']
  event_time = event_elem.attributes['time']
  event = Event.new(event_id, event_type, event_time)
  # Parse attributes
  event_elem.elements.each('attributes/attribute') do |attr_elem|
    attr_name = attr_elem.attributes['name']
    attr_value = attr_elem.text
    event_attribute = EventAttribute.new(attr_name, attr_value)
    event.attributes << event_attribute
  end
  # Parse relationships
  event_elem.elements.each('objects/relationship') do |rel_elem|
    rel_object_id = rel_elem.attributes['object-id']
    rel_qualifier = rel_elem.attributes['qualifier']
    relationship = Relationship.new(rel_object_id, rel_qualifier)
    event.relationships << relationship
  end
  events << event
end

# Now write to the output file
begin
  output = File.new(output_file, 'w')
  formatter = REXML::Formatters::Pretty.new
  formatter.compact = true
  outdoc = Document.new
  outdoc << XMLDecl.new('1.0', 'UTF-8')

  # Create root element
  log_elem = outdoc.add_element('log')

  # Write object-types
  ot_elem = log_elem.add_element('object-types')
  object_types.each do |object_type|
    ot = ot_elem.add_element('object-type', { 'name' => object_type.name })
    attrs_elem = ot.add_element('attributes')
    object_type.attributes.each do |attribute|
      attrs_elem.add_element('attribute', {
        'name' => attribute.name,
        'type' => attribute.type
      })
    end
  end

  # Write event-types
  et_elem = log_elem.add_element('event-types')
  event_types.each do |event_type|
    et = et_elem.add_element('event-type', { 'name' => event_type.name })
    attrs_elem = et.add_element('attributes')
    event_type.attributes.each do |attribute|
      attrs_elem.add_element('attribute', {
        'name' => attribute.name,
        'type' => attribute.type
      })
    end
  end

  # Write objects
  objects_elem = log_elem.add_element('objects')
  objects.each do |object|
    obj_elem = objects_elem.add_element('object', {
      'id' => object.id,
      'type' => object.type
    })
    # Attributes
    attrs_elem = obj_elem.add_element('attributes')
    object.attributes.each do |attribute|
      attr_elem = attrs_elem.add_element('attribute', {
        'name' => attribute.name,
        'time' => attribute.time
      })
      attr_elem.text = attribute.value
    end
    # Relationships
    unless object.relationships.empty?
      rels_elem = obj_elem.add_element('objects')
      object.relationships.each do |rel|
        rels_elem.add_element('relationship', {
          'object-id' => rel.object_id,
          'qualifier' => rel.qualifier
        })
      end
    end
  end

  # Write events
  events_elem = log_elem.add_element('events')
  events.each do |event|
    event_elem = events_elem.add_element('event', {
      'id' => event.id,
      'type' => event.type,
      'time' => event.time
    })
    # Attributes
    attrs_elem = event_elem.add_element('attributes')
    event.attributes.each do |attribute|
      attr_elem = attrs_elem.add_element('attribute', {
        'name' => attribute.name
      })
      attr_elem.text = attribute.value
    end
    # Relationships
    unless event.relationships.empty?
      rels_elem = event_elem.add_element('objects')
      event.relationships.each do |rel|
        rels_elem.add_element('relationship', {
          'object-id' => rel.object_id,
          'qualifier' => rel.qualifier
        })
      end
    end
  end

  formatter.write(outdoc, output)
  output.close
rescue Exception => e
  puts "Error: #{e}"
  exit
end

puts "Conversion completed successfully."