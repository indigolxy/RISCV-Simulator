
#ifndef RISCV_SIMULATOR_CIRCULAR_QUEUE_H
#define RISCV_SIMULATOR_CIRCULAR_QUEUE_H

template <typename T, int size>
class CircularQueue {
public:

  class iterator {
    friend class CircularQueue<T, size>;
  public:
    iterator() = default;
    iterator(int index, CircularQueue<T, size> *q) : index(index), q(q) {}
    iterator(const CircularQueue<T, size>::iterator &other) {
      index = other.index;
      q = other.q;
    }
    iterator &operator++() {
      index = (index + 1) % size;
      return *this;
    }
    iterator &operator--() {
      index = (index - 1) % size;
      return *this;
    }
    bool operator==(const iterator &other) {
      return index == other.index && q == other.q;
    }
    bool operator!=(const iterator &other) {
      return index != other.index && q == other.q;
    }

    T &operator*() const {
      return q->data[index];
    }

    T *operator->() const {
      return &q->data[index];
    }

  private:
    int index;
    CircularQueue<T, size> *q;
  };

public:
  CircularQueue() = default;

  CircularQueue &operator=(const CircularQueue<T, size> &other) {
    if (this == &other) return *this;
    for (int i = 0; i < size ; ++i) {
      data[i] = other.data[i];
    }
    head = other.head;
    tail = other.tail;
    cnt = other.cnt;
    return *this;
  }

  bool full() {
    return (tail + 1) % size == head;
  }

  // push at back
  int push(const T &obj) {
    data[tail] = obj;
    tail = (tail + 1) % size;
    return cnt++;
  }

  // pop at front
  void pop() {
    tail = (tail - 1 + size) % size;
  }

  iterator back() {
    return {tail - 1, this};
  }

  iterator end() {
    return {tail, this};
  }

  iterator front() {
    return {head, this};
  }

  iterator find(int index) {
    return {index % size, this};
  }

private:
  T data[size];
  int head = 0;
  int tail = 0;
  int cnt = 0;
};

#endif //RISCV_SIMULATOR_CIRCULAR_QUEUE_H
