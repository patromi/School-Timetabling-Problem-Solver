import xml.etree.ElementTree as ET
import numpy as np
from time_decoder import TimeDecoder

class ProblemParser:
    def __init__(self, xml_string):
        self.root = ET.fromstring(xml_string)
        self.problem_node = self.root 
        
        # Global parameters
        self.nr_days = int(self.problem_node.attrib['nrDays'])
        self.nr_weeks = int(self.problem_node.attrib['nrWeeks'])
        self.slots_per_day = int(self.problem_node.attrib['slotsPerDay'])
        
        # TimeDecoder instance to decode time options
        self.time_decoder = TimeDecoder(self.nr_days, self.nr_weeks, self.slots_per_day)
        
        # Mappings and data structures
        self.room_map = {}  # { 'room_id_str' : int_index }
        self.class_map = {} # { 'class_id_str' : int_index }
        self.course_map = {} # { 'course_id_str' : int_index }
        
        self.room_capacities = []
        self.classes_data = [] 
        self.distributions_data = [] 
        self.students_data = []

    def parse_rooms(self):
        """Parsuje sale, nadaje im integer indeksy i zwraca tablice Numpy pojemności."""
        rooms_node = self.problem_node.find('rooms')
        if rooms_node is None:
            return
            
        for idx, room in enumerate(rooms_node.findall('room')):
            room_id_str = room.attrib['id']
            self.room_map[room_id_str] = idx
            self.room_capacities.append(int(room.attrib['capacity']))
            

        return np.array(self.room_capacities, dtype=np.int32)

    def parse_classes(self):
        """Parsuje lekcje, korzystając z time_decodera do konwersji bitmasek."""
        courses_node = self.problem_node.find('courses')
        if courses_node is None:
            return

        class_idx = 0
        course_idx = 0
       
       # Go through each course, then each config, subpart, and class to extract relevant data
        for course in courses_node.findall('course'):
            course_id_str = course.attrib['id']
            self.course_map[course_id_str] = course_idx
            course_idx += 1

            for config in course.findall('config'):
                for subpart in config.findall('subpart'):
                    for cls in subpart.findall('class'):
                        cls_id_str = cls.attrib['id']
                        self.class_map[cls_id_str] = class_idx
                        
                        # Read allowed rooms for this class
                        allowed_rooms = []
                        for r in cls.findall('room'):
                            r_idx = self.room_map[r.attrib['id']]
                            allowed_rooms.append({
                                'room_idx': r_idx,
                                'penalty': int(r.attrib['penalty'])
                            })
                            
                        # 2. Read allowed times for this class
                        allowed_times = []
                        for t in cls.findall('time'):
                           
                            slots_array = self.time_decoder.decode_time_option(
                                days_mask=t.attrib['days'],
                                weeks_mask=t.attrib['weeks'],
                                start_slot=int(t.attrib['start']),
                                length=int(t.attrib['length'])
                            )
                            
                            allowed_times.append({
                                'absolute_slots': slots_array,
                                'penalty': int(t.attrib.get('penalty', 0))
                            })
                            
                      
                        self.classes_data.append({
                            'limit': int(cls.attrib.get('limit', 0)),
                            'allowed_rooms': allowed_rooms,
                            'allowed_times': allowed_times
                        })
                        
                        class_idx += 1
                        
        return self.classes_data
    
    def parse_distributions(self):
        """Parsuje ograniczenia twarde i miękkie pomiędzy konkretnymi zajęciami."""
        dist_node = self.problem_node.find('distributions')
        if dist_node is None:
            return []
            
        for dist in dist_node.findall('distribution'):
            dist_type = dist.attrib['type']
        
            is_required = dist.attrib.get('required', 'false').lower() == 'true'
            penalty = int(dist.attrib.get('penalty', 0))
            
            # Map class IDs to their integer indices
            involved_classes = []
            for c in dist.findall('class'):
                c_id_str = c.attrib['id']
                if c_id_str in self.class_map:
                    involved_classes.append(self.class_map[c_id_str])
                    
            self.distributions_data.append({
                'type': dist_type,
                'is_required': is_required,
                'penalty': penalty,
                'classes': involved_classes
            })
            
        return self.distributions_data
    

    def parse_students(self):
        """Parsuje zapotrzebowanie studentów na kursy."""
        students_node = self.problem_node.find('students')
        if students_node is None:
            return []
            
        for student in students_node.findall('student'):
            student_id = student.attrib['id']
            
            requested_courses = []
            for c in student.findall('course'):
                c_id_str = c.attrib['id']
                if c_id_str in self.course_map:
                    requested_courses.append(self.course_map[c_id_str])
                    
            self.students_data.append({
                'student_id': student_id,
                'courses': requested_courses
            })
            
        return self.students_data

    def run_parser(self):
        """Uruchamia pełny proces parsowania i zwraca gotowe dane."""
        capacities_arr = self.parse_rooms()
        classes_info = self.parse_classes()
        distributions_info = self.parse_distributions()
        students_info = self.parse_students()
        
        return {
            'room_capacities': capacities_arr,
            'classes': classes_info,
            'distributions': distributions_info,
            'students': students_info,
            'total_slots': self.time_decoder.total_slots,
            'num_rooms': len(self.room_map),
            'num_classes': len(self.class_map)
        }



#Teścik 

test_path = 'sample_data/wbg-fal10.xml'


with open(test_path, 'r', encoding='utf-8') as f:
    xml_data = f.read()

parser = ProblemParser(xml_data)
parsed_data = parser.run_parser()

with open('test.txt', 'w', encoding='utf-8') as f: #XD
    f.write(parsed_data.__str__())
