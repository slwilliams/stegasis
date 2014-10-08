class SteganographicAlgorithm {                                                 
  public:                                                                       
    virtual void embed(*byte[] frame, byte[] data) = 0;                         
    virtual *byte[] extract(*byte[] frame) = 0;                                 
};
