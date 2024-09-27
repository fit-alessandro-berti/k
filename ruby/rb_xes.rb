# signature of XESImporterExporter (implemented in rb_xes.rb)
#
# constructor: XESImporterExporter.new
#
# methods:
# * importer_exporter.import(input_file)
# * importer_exporter.export(output_file)

class XESImporterExporter
  attr_accessor :traces

  def initialize
    @traces = []
  end

  # Import XES file and populate @traces
  def import(file_path)
    current_trace = nil
    inside_event = false

    File.open(file_path, 'r') do |file|
      file.each_line do |line|
        line.strip!

        if line.start_with?('<trace>')
          current_trace = []
        elsif line.start_with?('</trace>')
          @traces << current_trace if current_trace
          current_trace = nil
        elsif line.start_with?('<event>')
          inside_event = true
        elsif line.start_with?('</event>')
          inside_event = false
        elsif inside_event && line.start_with?('<string ')
          # Extract key and value
          key = extract_attribute(line, 'key')
          value = extract_attribute(line, 'value')
          if key == 'concept:name'
            current_trace << value if current_trace
          end
        end
      end
    end
  end

  # Export @traces to XES file
  def export(file_path)
    File.open(file_path, 'w') do |file|
      # Write XML header
      file.puts("<?xml version='1.0' encoding='UTF-8'?>")
      # Start log tag
      file.puts('<log>')

      @traces.each do |trace|
        # Start trace
        file.puts('  <trace>')
        trace.each do |activity|
          # Start event
          file.puts('    <event>')
          # Write concept:name
          file.puts("      <string key='concept:name' value='#{escape_xml(activity)}'/>")
          # End event
          file.puts('    </event>')
        end
        # End trace
        file.puts('  </trace>')
      end

      # End log tag
      file.puts('</log>')
    end
  end

  private

  # Helper method to extract attribute values from a tag
  def extract_attribute(line, attr_name)
    if match = line.match(/#{attr_name}='([^']*)'/)
      match[1]
    elsif match = line.match(/#{attr_name}="([^"]*)"/)
      match[1]
    else
      nil
    end
  end

  # Escape special XML characters
  def escape_xml(text)
    text.to_s.gsub('&', '&amp;').gsub('<', '&lt;').gsub('>', '&gt;')
             .gsub('"', '&quot;').gsub("'", '&apos;')
  end
end

# Main execution: handle command-line arguments
if __FILE__ == $0
  if ARGV.length != 2
    puts "Usage: ruby #{__FILE__} input_file.xes output_file.xes"
    exit 1
  end

  input_file = ARGV[0]
  output_file = ARGV[1]

  # Check if input file exists
  unless File.exist?(input_file)
    puts "Error: Input file '#{input_file}' does not exist."
    exit 1
  end

  # Create an instance of the importer/exporter
  importer_exporter = XESImporterExporter.new

  # Perform import and export
  begin
    puts "Importing from '#{input_file}'..."
    importer_exporter.import(input_file)
    puts "Successfully imported #{importer_exporter.traces.size} traces."

    puts "Exporting to '#{output_file}'..."
    importer_exporter.export(output_file)
    puts "Export completed."
  rescue => e
    puts "An error occurred: #{e.message}"
    exit 1
  end
end