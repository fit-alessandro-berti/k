require 'json'
require 'time'

# Classes to represent the OCEL components
class EventType
  attr_accessor :name, :attributes

  def initialize(name, attributes)
    @name = name
    @attributes = attributes || []
  end

  def to_hash
    {
      'name' => @name,
      'attributes' => @attributes
    }
  end
end

class ObjectType
  attr_accessor :name, :attributes

  def initialize(name, attributes)
    @name = name
    @attributes = attributes || []
  end

  def to_hash
    {
      'name' => @name,
      'attributes' => @attributes
    }
  end
end

class Event
  attr_accessor :id, :type, :time, :attributes, :relationships

  def initialize(id, type, time, attributes, relationships)
    @id = id
    @type = type
    @time = time
    @attributes = attributes || []
    @relationships = relationships || []
  end

  def to_hash
    {
      'id' => @id,
      'type' => @type,
      'time' => @time,
      'attributes' => @attributes,
      'relationships' => @relationships
    }
  end
end

class ObjectInstance
  attr_accessor :id, :type, :attributes, :relationships

  def initialize(id, type, attributes, relationships)
    @id = id
    @type = type
    @attributes = attributes || []
    @relationships = relationships || []
  end

  def to_hash
    hash = {
      'id' => @id,
      'type' => @type,
    }
    hash['attributes'] = @attributes if @attributes && !@attributes.empty?
    hash['relationships'] = @relationships if @relationships && !@relationships.empty?
    hash
  end
end

# Main script execution starts here
if ARGV.length != 2
  puts "Usage: ruby ocel_import_export.rb input_file.json output_file.json"
  exit 1
end

input_file = ARGV[0]
output_file = ARGV[1]

# Read and parse the input OCEL JSON file
begin
  json_data = File.read(input_file)
  data = JSON.parse(json_data)
rescue => e
  puts "Error reading or parsing input file: #{e.message}"
  exit 1
end

# Parse object types
object_types = []
if data['objectTypes']
  data['objectTypes'].each do |ot|
    name = ot['name']
    attributes = ot['attributes']
    object_types << ObjectType.new(name, attributes)
  end
end

# Parse event types
event_types = []
if data['eventTypes']
  data['eventTypes'].each do |et|
    name = et['name']
    attributes = et['attributes']
    event_types << EventType.new(name, attributes)
  end
end

# Parse objects
objects = []
if data['objects']
  data['objects'].each do |obj|
    id = obj['id']
    type = obj['type']
    attributes = obj['attributes']
    relationships = obj['relationships']
    objects << ObjectInstance.new(id, type, attributes, relationships)
  end
end

# Parse events
events = []
if data['events']
  data['events'].each do |ev|
    id = ev['id']
    type = ev['type']
    time = ev['time']
    attributes = ev['attributes']
    relationships = ev['relationships']
    events << Event.new(id, type, time, attributes, relationships)
  end
end

# Reconstruct the data into a hash to be written as JSON
output_data = {
  'objectTypes' => object_types.map(&:to_hash),
  'eventTypes' => event_types.map(&:to_hash),
  'objects' => objects.map(&:to_hash),
  'events' => events.map(&:to_hash)
}

# Write the output OCEL JSON file
begin
  File.open(output_file, 'w') do |file|
    file.write(JSON.pretty_generate(output_data))
  end
rescue => e
  puts "Error writing output file: #{e.message}"
  exit 1
end

puts "Successfully processed #{input_file} and wrote output to #{output_file}"