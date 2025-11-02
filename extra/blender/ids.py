class IdMapper():
    def __init__(self):
        self.store = {}
        self.largest_id = 0

    def get(self, id):
        return self.store.get(id, None)

    def map_existing(self, id, val):
        self.store[id] = val
        self.largest_id = max(self.largest_id, id)
    
    def map_new(self, val):
        self.largest_id+=1
        self.store[self.largest_id] = val
        return self.largest_id

    def unmap(self, id):
        if id in self.store:
            del self.store[id]
    
id_mapper = IdMapper()