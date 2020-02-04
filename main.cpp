
#include <stdio.h>
#include <unistd.h>
#include "bcm2835.h" // https://www.airspayce.com/mikem/bcm2835/

// GPIO pin for I2C
#define I2C_SCL_PIN RPI_V2_GPIO_P1_05
#define I2C_SDA_PIN RPI_V2_GPIO_P1_03

// software I2C
class I2C {
private:
	void delay()
	{
		// Bit-bang方式によるオーバーヘッドで充分なウェイトが得られるので何もしない
		// usleep(10);
	}

	// 初期化
	void init_i2c()
	{
		// SCLピンを入力モードにする
		bcm2835_gpio_fsel(I2C_SCL_PIN, BCM2835_GPIO_FSEL_INPT);
		// SDAピンを入力モードにする
		bcm2835_gpio_fsel(I2C_SDA_PIN, BCM2835_GPIO_FSEL_INPT);
		// SCLピンをLOWにする
		bcm2835_gpio_clr(I2C_SCL_PIN);
		// SDAピンをLOWにする
		bcm2835_gpio_clr(I2C_SDA_PIN);
	}

	void i2c_cl_0()
	{
		// SCLピンを出力モードにするとピンの状態がLOWになる
		bcm2835_gpio_fsel(I2C_SCL_PIN, BCM2835_GPIO_FSEL_OUTP);
	}

	void i2c_cl_1()
	{
		// SCLピンを入力モードにするとピンの状態がHIGHになる
		bcm2835_gpio_fsel(I2C_SCL_PIN, BCM2835_GPIO_FSEL_INPT);
	}

	void i2c_da_0()
	{
		// SDAピンを出力モードにするとピンの状態がLOWになる
		bcm2835_gpio_fsel(I2C_SDA_PIN, BCM2835_GPIO_FSEL_OUTP);
	}

	void i2c_da_1()
	{
		// SDAピンを入力モードにするとピンの状態がHIGHになる
		bcm2835_gpio_fsel(I2C_SDA_PIN, BCM2835_GPIO_FSEL_INPT);
	}

	int i2c_get_da()
	{
		// SDAピンの状態を読み取る
		return bcm2835_gpio_lev(I2C_SDA_PIN) ? 1 : 0;
	}

	// スタートコンディション
	void i2c_start()
	{
		i2c_da_0(); // SDA=0
		delay();
		i2c_cl_0(); // SCL=0
		delay();
	}

	// ストップコンディション
	void i2c_stop()
	{
		i2c_cl_1(); // SCL=1
		delay();
		i2c_da_1(); // SDA=1
		delay();
	}

	// リピーテッドスタートコンディション
	void i2c_repeat()
	{
		i2c_cl_1(); // SCL=1
		delay();
		i2c_da_0(); // SDA=0
		delay();
		i2c_cl_0(); // SCL=0
		delay();
	}

	// 1バイト送信
	bool i2c_write(int c)
	{
		int i;
		bool nack;

		delay();

		// 8ビット送信
		for (i = 0; i < 8; i++) {
			if (c & 0x80) {
				i2c_da_1(); // SCL=1
			} else {
				i2c_da_0(); // SCL=0
			}
			c <<= 1;
			delay();
			i2c_cl_1(); // SCL=1
			delay();
			i2c_cl_0(); // SCL=0
			delay();
		}

		i2c_da_1(); // SDA=1
		delay();

		i2c_cl_1(); // SCL=1
		delay();
		// NACKビットを受信
		nack = i2c_get_da();
		i2c_cl_0(); // SCL=0

		return nack;
	}

	// 1バイト受信
	int i2c_read(bool nack)
	{
		int i, c;

		i2c_da_1(); // SDA=1
		delay();

		c = 0;

		for (i = 0; i < 8; i++) {
			i2c_cl_1(); // SCL=1
			delay();
			c <<= 1;
			if (i2c_get_da()) { // SDAから1ビット受信
				c |= 1;
			}
			i2c_cl_0(); // SCL=0
			delay();
		}

		// NACKビットを送信
		if (nack) {
			i2c_da_1(); // SDA=1
		} else {
			i2c_da_0(); // SDA=0
		}
		delay();
		i2c_cl_1(); // SCL=1
		delay();
		i2c_cl_0(); // SCL=0
		delay();

		return c;
	}

	int address; // I2Cデバイスアドレス

public:
	I2C(int address)
		: address(address)
	{
		init_i2c();
	}

	bool scan(int addr)
	{
		i2c_start();                   // スタート
		bool nack = i2c_write(addr << 1);       // デバイスアドレスを送信
		i2c_stop();                    // ストップ
		return !nack;
	}

	// デバイスのレジスタに書き込む
	void write(int reg, int data)
	{
		i2c_start();                   // スタート
		i2c_write(address << 1);       // デバイスアドレスを送信
		if (reg >= 0) {
			i2c_write(reg);            // レジスタ番号を送信
		}
		i2c_write(data);               // データを送信
		i2c_stop();                    // ストップ
	}

	// デバイスのレジスタを読み取る
	int read(int reg)
	{
		int data;
		i2c_start();                   // スタート
		i2c_write(address << 1);       // デバイスアドレスを送信
		i2c_write(reg);                // レジスタ番号を送信
		i2c_repeat();                  // リピーテッドスタートコンディション
		i2c_write((address << 1) | 1); // デバイスアドレスを送信（読み取りモード）
		data = i2c_read(true);         // データを受信
		i2c_stop();                    // 受信
		return data;
	}
};





int main()
{
	bcm2835_init();
	I2C i2c(0);
	
	for (int i = 0; i < 128; i++) {
		bool ack = i2c.scan(i);
		if (ack) {
			printf("%02x", i);
		} else {
			printf("--");
		}
		putchar((i % 16 < 15) ? ' ' : '\n');
	}

	return 0;
}

