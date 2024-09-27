require 'rexml/document'
include REXML

# signatures in rb_pnml.rb
#

# Define the Place class
class Place
  attr_accessor :id, :name, :initial_marking

  def initialize(id, name)
    @id = id
    @name = name
    @initial_marking = nil
  end
end

# Define the Transition class
class Transition
  attr_accessor :id, :name, :visible, :local_node_id

  def initialize(id, name)
    @id = id
    @name = name
    @visible = true # Transitions are visible by default
    @local_node_id = ''
  end
end

# Define the Arc class
class Arc
  attr_accessor :id, :source_id, :target_id

  def initialize(id, source_id, target_id)
    @id = id
    @source_id = source_id
    @target_id = target_id
  end
end

# Define the PetriNet class
class PetriNet
  attr_accessor :places, :transitions, :arcs, :initial_markings, :final_markings

  def initialize
    @places = {}
    @transitions = {}
    @arcs = []
    @initial_markings = {}
    @final_markings = {}
  end

  def add_place(place)
    @places[place.id] = place
  end

  def add_transition(transition)
    @transitions[transition.id] = transition
  end

  def add_arc(arc)
    @arcs << arc
  end
end

# Function to parse the PNML file and build the Petri net
def parse_pnml(file)
  petri_net = PetriNet.new

  doc = REXML::Document.new(File.read(file))

  # Assuming single <net> and <page> elements
  net = doc.elements['pnml/net']
  raise "No <net> element found in PNML file." unless net

  # Parse places
  net.elements.each('page/place') do |place_element|
    id = place_element.attributes['id']
    name = place_element.elements['name/text'].text rescue ''
    place = Place.new(id, name)

    if place_element.elements['initialMarking/text']
      initial_token = place_element.elements['initialMarking/text'].text.to_i
      place.initial_marking = initial_token
      petri_net.initial_markings[id] = initial_token
    end

    petri_net.add_place(place)
  end

  # Parse transitions
  net.elements.each('page/transition') do |transition_element|
    id = transition_element.attributes['id']
    name = transition_element.elements['name/text'].text rescue ''
    transition = Transition.new(id, name)

    # Check for <toolspecific> element indicating invisibility
    toolspecific = transition_element.elements['toolspecific']
    if toolspecific && toolspecific.attributes['activity'] == '$invisible$'
      transition.visible = false
      transition.local_node_id = toolspecific.attributes['localNodeID'] || ''
    end

    petri_net.add_transition(transition)
  end

  # Parse arcs
  net.elements.each('page/arc') do |arc_element|
    id = arc_element.attributes['id']
    source = arc_element.attributes['source']
    target = arc_element.attributes['target']
    arc = Arc.new(id, source, target)
    petri_net.add_arc(arc)
  end

  # Parse final markings
  net.elements.each('finalmarkings/marking/place') do |place_element|
    idref = place_element.attributes['idref']
    tokens = place_element.elements['text'].text.to_i
    petri_net.final_markings[idref] = tokens
  end

  petri_net
end

# Function to write the Petri net back to a PNML file
def write_pnml(petri_net, file)
  doc = Document.new
  doc << XMLDecl.new('1.0', 'UTF-8')

  pnml = doc.add_element('pnml')
  net = pnml.add_element('net', {
    'id' => 'net1',
    'type' => 'http://www.pnml.org/version-2009/grammar/pnmlcoremodel'
  })
  name = net.add_element('name')
  name.add_element('text').text = 'net1'

  page = net.add_element('page', {'id' => 'n0'})

  # Write places
  petri_net.places.values.each do |place|
    place_element = page.add_element('place', {'id' => place.id})
    name_element = place_element.add_element('name')
    name_element.add_element('text').text = place.name
    if place.initial_marking
      initial_marking = place_element.add_element('initialMarking')
      initial_marking.add_element('text').text = place.initial_marking.to_s
    end
  end

  # Write transitions
  petri_net.transitions.values.each do |transition|
    trans_element = page.add_element('transition', {'id' => transition.id})
    name_element = trans_element.add_element('name')
    name_element.add_element('text').text = transition.name
    unless transition.visible
      trans_element.add_element('toolspecific', {
        'tool' => 'ProM',
        'version' => '6.4',
        'activity' => '$invisible$',
        'localNodeID' => transition.local_node_id
      })
    end
  end

  # Write arcs
  petri_net.arcs.each do |arc|
    arc_element = page.add_element('arc', {
      'id' => arc.id,
      'source' => arc.source_id,
      'target' => arc.target_id
    })
  end

  # Write final markings
  unless petri_net.final_markings.empty?
    finalmarkings = net.add_element('finalmarkings')
    marking = finalmarkings.add_element('marking')
    petri_net.final_markings.each do |place_id, tokens|
      place_element = marking.add_element('place', {'idref' => place_id})
      place_element.add_element('text').text = tokens.to_s
    end
  end

  # Write the XML document to file
  File.open(file, 'w') do |f|
    formatter = REXML::Formatters::Pretty.new
    formatter.compact = true # This makes the XML more compact
    formatter.write(doc, f)
  end
end

if __FILE__ == $0
	# Main script execution
	if ARGV.length != 2
	  puts "Usage: ruby script.rb input.pnml output.pnml"
	  exit 1
	end

	input_file = ARGV[0]
	output_file = ARGV[1]

	# Parse input PNML file
	petri_net = parse_pnml(input_file)

	# You can manipulate the Petri net here if needed
	# For example, to add a new place:
	# new_place = Place.new('p_new', 'New Place')
	# petri_net.add_place(new_place)

	# Write the Petri net back to an output PNML file
	write_pnml(petri_net, output_file)
end