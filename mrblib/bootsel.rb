class BOOTSEL
  def read
    mrbc_gpio_get
  end

  def high?
    read == 1
  end

  def low?
    read == 0
  end
end
